#include <xc.h>
#include <stdint.h>

// ========================================================
// 1. CONFIGURACI�N DE LOS FUSIBLES (BITS DE CONFIGURACI�N)
// ========================================================
#pragma config FOSC = INTOSCIO  // Oscilador Interno a 4MHz, pines RA6 y RA7 como I/O
#pragma config WDTE = OFF       // Watchdog Timer por hardware desactivado
#pragma config PWRTE = ON       // Power-up Timer activado
#pragma config MCLRE = OFF      // Pin MCLR como entrada digital (RA5)
#pragma config BOREN = ON       // Brown-out Detect activado
#pragma config LVP = OFF        // Low-Voltage Programming desactivado
#pragma config CPD = OFF        // Protecci�n de datos EEPROM desactivada
#pragma config CP = OFF         // Protecci�n de c�digo desactivada

#define _XTAL_FREQ 4000000      // Frecuencia de 4 MHz

// ========================================================
// 2. CONSTANTES Y DEFINICIONES (PROTOCOLOS Y PINES)
// ========================================================
// Constantes del Protocolo (Fase 3)
#define HEADER_RX 0xAA
#define HEADER_TX 0xBB
#define TAIL      0x55

// Estados del Motor
#define ST_READY  0x00
#define ST_MOVING 0x01
#define ST_HOMING 0x02
#define ST_ERROR  0xEE

// Comandos esperados
#define CMD_HOMING 0x01
#define CMD_GOTO   0x02

// Definici�n de Pines de Control del Motor (Fase 2)
#define DIR_A       PORTBbits.RB3   // Sentido A (Compartido)
#define DIR_B       PORTAbits.RA1   // Sentido B (Compartido)
#define ENABLE_M1   PORTBbits.RB5   // Enable Motor 1
#define ENABLE_M2   PORTBbits.RB4   // Enable Motor 2
#define SENS1_M1    PORTAbits.RA2   // Encoder Muescas M1
#define SENS2_M1    PORTAbits.RA3   // Encoder Homing M1
#define SENS1_M2    PORTAbits.RA7   // Encoder Muescas M2
#define SENS2_M2    PORTAbits.RA0   // Encoder Homing M2

// ========================================================
// 3. VARIABLES GLOBALES
// ========================================================
// Variables de Comunicaci�n UART
volatile uint8_t buffer_rx[6];
volatile uint8_t indice_rx = 0;
volatile uint8_t trama_lista = 0;

// Variables de Estado F�sico
volatile uint8_t estado_actual = ST_READY;
int posicion_m1 = 0;
int posicion_m2 = 0;

// ========================================================
// 4. INICIALIZACI�N DE HARDWARE Y UART
// ========================================================
void inicializar_hardware() {
    CMCON = 0x07; // Apagar comparadores anal�gicos (obligatorio para I/O digital)

    // Configuraci�n de Entradas y Salidas
    TRISBbits.TRISB3 = 0; // DIR_A
    TRISAbits.TRISA1 = 0; // DIR_B
    TRISBbits.TRISB5 = 0; // ENABLE_M1
    TRISBbits.TRISB4 = 0; // ENABLE_M2
    TRISAbits.TRISA2 = 1; // SENS1_M1
    TRISAbits.TRISA3 = 1; // SENS2_M1
    TRISAbits.TRISA7 = 1; // SENS1_M2
    TRISAbits.TRISA0 = 1; // SENS2_M2

    // Estados iniciales seguros
    ENABLE_M1 = 0;
    ENABLE_M2 = 0;
    DIR_A = 0;
    DIR_B = 0;
}

void UART_Init() {
    TRISB1 = 1; // RB1/RX como entrada
    TRISB2 = 0; // RB2/TX como salida

    // Configuraci�n para 9600 bps con Fosc = 4 MHz
    SPBRG = 25;
    TXSTA = 0b00100100; // Transmisi�n (TXEN=1), Alta velocidad (BRGH=1)
    RCSTA = 0b10010000; // Puerto serial (SPEN=1), recepci�n continua (CREN=1)

    // Interrupciones
    RCIE = 1;   // Interrupci�n de recepci�n UART
    PEIE = 1;   // Interrupciones de perif�ricos
    GIE = 1;    // Interrupciones globales
}

void UART_Write(uint8_t data) {
    while (!TRMT); // Esperar a que el buffer de transmisi�n est� libre
    TXREG = data;
}

// Funci�n auxiliar para enviar respuestas estructuradas al ESP32
void enviar_respuesta(uint8_t id_motor, uint8_t status, uint8_t pos) {
    uint8_t chk_tx = id_motor ^ status ^ pos;
    UART_Write(HEADER_TX);      // 0xBB
    UART_Write(id_motor);
    UART_Write(status);
    UART_Write(pos);
    UART_Write(chk_tx);
    UART_Write(TAIL);           // 0x55
}

// ========================================================
// 5. RUTINAS DE CONTROL DE MOTORES
// ========================================================
void frenado_dinamico_m1() {
    DIR_A = 1;
    DIR_B = 1;
    ENABLE_M1 = 1;
    __delay_ms(50);
    ENABLE_M1 = 0;
    DIR_A = 0;
    DIR_B = 0;
}

void homing_m1() {
    DIR_A = 1;
    DIR_B = 0;
    int pulsos_on = 11;
    int pulsos_off = 31;

    while(1) {
        ENABLE_M1 = 1;
        for(int i = 0; i < pulsos_on; i++) {
            if(SENS2_M1 == 0) {
                frenado_dinamico_m1();
                posicion_m1 = 1;
                return;
            }
            __delay_us(100);
        }

        ENABLE_M1 = 0;
        for(int i = 0; i < pulsos_off; i++) {
            if(SENS2_M1 == 0) {
                frenado_dinamico_m1();
                posicion_m1 = 1;
                return;
            }
            __delay_us(100);
        }
    }
}

void ir_a_posicion_m1(int posicion_destino) {
    int pasos_a_dar = posicion_destino - posicion_m1;
    if (pasos_a_dar == 0) return;

    if (pasos_a_dar > 0) {
        DIR_A = 1; DIR_B = 0;
    } else {
        DIR_A = 0; DIR_B = 1;
        pasos_a_dar = -pasos_a_dar;
    }

    int ranuras_objetivo = pasos_a_dar * 5;
    int ranuras_contadas = 0;
    int estado_anterior = SENS1_M1;

    int pulsos_on = 11;
    int pulsos_off = 31;
    int ciclo_pwm = 0;

    unsigned int timeout_contador = 0;
    const unsigned int TIMEOUT_MAX = 30000; // Aprox 5s

    while(1) {
        // --- WATCHDOG CONTRA ATASCOS (MODIFICADO PARA FASE 3) ---
        timeout_contador++;
        if (timeout_contador >= TIMEOUT_MAX) {
            frenado_dinamico_m1();
            estado_actual = ST_ERROR;

            // Si hay un atasco, avisa inmediatamente al ESP32 y aborta la funci�n
            enviar_respuesta(1, estado_actual, (uint8_t)posicion_m1);
            return;
        }

        ciclo_pwm++;
        if (ciclo_pwm <= pulsos_on) ENABLE_M1 = 1;
        else if (ciclo_pwm <= (pulsos_on + pulsos_off)) ENABLE_M1 = 0;
        else ciclo_pwm = 0;

        int lectura_actual = SENS1_M1;

        if (lectura_actual != estado_anterior) {
            timeout_contador = 0; // Reinicia el Watchdog si hay movimiento

            if (lectura_actual == 0) {
                ranuras_contadas++;

                if (ranuras_contadas == (ranuras_objetivo - 1)) {
                    pulsos_on = 5; pulsos_off = 36;
                }

                if (ranuras_contadas >= ranuras_objetivo) {
                    int dir_a_original = DIR_A;
                    int dir_b_original = DIR_B;

                    frenado_dinamico_m1();
                    __delay_ms(15);

                    if (SENS1_M1 == 1) {
                        DIR_A = dir_a_original;
                        DIR_B = dir_b_original;

                        while(SENS1_M1 == 1) {
                            ENABLE_M1 = 1;
                            __delay_ms(15);
                            ENABLE_M1 = 0;
                            __delay_ms(20);
                        }
                        frenado_dinamico_m1();
                    }
                    posicion_m1 = posicion_destino;
                    return;
                }
            }
            estado_anterior = lectura_actual;
        }
        __delay_us(100);
    }
}

// ========================================================
// 6. PROCESAMIENTO DE TRAMA UART (PARSER)
// ========================================================
void procesarTrama() {
    uint8_t id_motor = buffer_rx[1];
    uint8_t cmd      = buffer_rx[2];
    uint8_t param    = buffer_rx[3];
    uint8_t chk_rx   = buffer_rx[4];

    // Validaci�n mediante Checksum
    if ((id_motor ^ cmd ^ param) == chk_rx) {

        if (id_motor == 0x01) { // Operamos sobre el Motor 1
            estado_actual = ST_READY;

            if (cmd == CMD_HOMING) {
                estado_actual = ST_HOMING;
                homing_m1();
            }
            else if (cmd == CMD_GOTO) {
                estado_actual = ST_MOVING;
                ir_a_posicion_m1(param);
            }

            // Si la funci�n de movimiento fall� por atasco, ya mand� el ERROR (0xEE)
            // Solo respondemos con �xito si el motor termin� en buen estado
            if (estado_actual != ST_ERROR) {
                estado_actual = ST_READY; // El motor termin� de moverse
                enviar_respuesta(id_motor, estado_actual, (uint8_t)posicion_m1);
            }
        }
    }
}

// ========================================================
// 7. RUTINA DE INTERRUPCI�N (ISR)
// ========================================================
void __interrupt() isr_routine(void) {
    if (RCIF) {
        // Manejo de Error Overrun
        if (OERR) {
            CREN = 0;
            CREN = 1;
        }

        uint8_t dato = RCREG;

        // Sincronizaci�n estricta
        if (dato == HEADER_RX) {
            indice_rx = 0;
        }

        if (indice_rx < 6) {
            buffer_rx[indice_rx++] = dato;

            if (indice_rx == 6) {
                if (buffer_rx[5] == TAIL) {  // Validar cola
                    trama_lista = 1;         // Avisar al main()
                }
                indice_rx = 0;               // Reiniciar buffer
            }
        }
    }
}

// ========================================================
// 8. BUCLE PRINCIPAL (MAIN)
// ========================================================
void main(void) {
    inicializar_hardware();
    UART_Init();

    // Pausa de estabilizaci�n al arranque
    __delay_ms(10000);

    // El ESP32 es el maestro absoluto. El PIC solo espera instrucciones.
    while (1) {
        if (trama_lista) {
            procesarTrama();
            trama_lista = 0; // Limpiar bandera tras procesar
        }
    }
}