#include <xc.h>
#include <stdlib.h> // Librería necesaria para usar rand() en la prueba de estrés

// ========================================================
// 1. CONFIGURACIÓN DE LOS FUSIBLES (BITS DE CONFIGURACIÓN)
// ========================================================
#pragma config FOSC = INTOSCIO  // Oscilador interno de 4MHz. Libera RA6 y RA7 como pines digitales I/O.
#pragma config WDTE = OFF       // Watchdog de hardware apagado (usamos un Watchdog por software de 5s).
#pragma config PWRTE = ON       // Power-up Timer encendido (da tiempo a que el voltaje suba antes de arrancar).
#pragma config MCLRE = OFF      // Master Clear apagado. El pin RA5 funciona como entrada digital.
#pragma config BOREN = ON       // Brown-out Reset encendido (reinicia el PIC si hay caídas de voltaje).
#pragma config LVP = OFF        // Low-Voltage Programming apagado. Obligatorio para liberar el pin RB4.
#pragma config CPD = OFF        // Protección de EEPROM apagada.
#pragma config CP = OFF         // Protección de Código apagada.

#define _XTAL_FREQ 4000000      // Define la frecuencia del oscilador (4 MHz) para que funcionen los __delay_ms()

// ========================================================
// 2. DEFINICIÓN DE PINES SEGÚN ESQUEMÁTICO FÍSICO
// ========================================================

// PINES DE DIRECCIÓN COMPARTIDOS (Hardware Multiplexing en el L293D)
// RB3 y RA1 controlan el sentido de giro de AMBOS motores a la vez (conectados a 1A/3A y 2A/4A).
#define DIR_A       PORTBbits.RB3   // Etiqueta 'M1' en el esquemático (Pin 9 del PIC)
#define DIR_B       PORTAbits.RA1   // Etiqueta 'M2' en el esquemático (Pin 18 del PIC)

// PINES DE HABILITACIÓN Y PWM (Seleccionan qué motor se mueve)
#define ENABLE_M1   PORTBbits.RB5   // Etiqueta 'M1_EN'. Activa el Motor 1 (Pin 11 del PIC)
#define ENABLE_M2   PORTBbits.RB4   // Etiqueta 'M2_EN'. Activa el Motor 2 (Pin 10 del PIC)

// PINES DE SENSORES ÓPTICOS (Entradas)
// Motor 1 (Canales 1 al 12)
#define SENS1_M1    PORTAbits.RA2   // Encoder de muescas continuas (Pin 1 del PIC)
#define SENS2_M1    PORTAbits.RA3   // Encoder de Homing / Cero absoluto (Pin 2 del PIC)
// Motor 2 (Canales 13 al 24)
#define SENS1_M2    PORTAbits.RA7   // Encoder de muescas continuas (Pin 16 del PIC)
#define SENS2_M2    PORTAbits.RA0   // Encoder de Homing / Cero absoluto (Pin 17 del PIC)

// VARIABLES GLOBALES DE POSICIÓN
int posicion_m1 = 0; // Memoria de la posición actual del selector 1 (1 a 12)
int posicion_m2 = 0; // Memoria de la posición actual del selector 2 (1 a 12)

// ========================================================
// 3. INICIALIZACIÓN DE HARDWARE Y REGISTROS
// ========================================================
void inicializar() {
    // CMCON = 0x07 apaga los módulos comparadores analógicos del PIC.
    // Esto es vital para que pines como RA0, RA1, RA2 y RA3 funcionen como entradas/salidas 100% digitales.
    CMCON = 0x07;       
    
    // Configuración del Registro TRIS (0 = Salida, 1 = Entrada)
    // Direcciones y Enables (Salidas hacia el L293D)
    TRISBbits.TRISB3 = 0; // DIR_A
    TRISAbits.TRISA1 = 0; // DIR_B
    TRISBbits.TRISB5 = 0; // ENABLE_M1
    TRISBbits.TRISB4 = 0; // ENABLE_M2
    
    // Sensores Ópticos (Entradas desde las herraduras infrarrojas)
    TRISAbits.TRISA2 = 1; // SENS1_M1
    TRISAbits.TRISA3 = 1; // SENS2_M1
    TRISAbits.TRISA7 = 1; // SENS1_M2
    TRISAbits.TRISA0 = 1; // SENS2_M2
 
    // Estados iniciales seguros: Todo apagado para evitar movimientos bruscos al energizar.
    ENABLE_M1 = 0; 
    ENABLE_M2 = 0; 
    DIR_A = 0; 
    DIR_B = 0;
}

// ========================================================
// 4. RUTINAS DE FRENADO DINÁMICO (Actividad 3)
// En lugar de quitar la energía, pone en cortocircuito lógico
// las bobinas del motor para frenarlo instantáneamente por 50ms.
// ========================================================
void frenado_dinamico_m1() {
    DIR_A = 1;
    DIR_B = 1;      // Envía 1 lógico a ambas entradas de dirección
    ENABLE_M1 = 1;  // Aplica el freno activando el puente
    __delay_ms(50); // Ventana de estabilización mecánica (Freno activo)
    ENABLE_M1 = 0;  // Libera el motor
    DIR_A = 0;
    DIR_B = 0;
}

void frenado_dinamico_m2() {
    DIR_A = 1;
    DIR_B = 1;      
    ENABLE_M2 = 1;  
    __delay_ms(50); 
    ENABLE_M2 = 0;  
    DIR_A = 0;
    DIR_B = 0;
}

// ========================================================
// 5. RUTINAS DE BÚSQUEDA DE CERO ABSOLUTO / HOMING (Actividad 1)
// Girar a baja velocidad hasta alinear el hueco del sensor Home.
// ========================================================
void homing_m1() {
    DIR_A = 1; // Sentido de giro predeterminado
    DIR_B = 0;
    
    // Ciclo de trabajo (Duty Cycle) para un giro lento
    int pulsos_on = 10;   
    int pulsos_off = 31; 

    while(1) {
        // Fase de encendido del PWM por software
        ENABLE_M1 = 1;
        for(int i = 0; i < pulsos_on; i++) {
            if(SENS2_M1 == 0) {     // Flanco de bajada detectado: Encontramos la muesca
                frenado_dinamico_m1(); // Clava el motor
                posicion_m1 = 1;       // Asigna la referencia espacial 1
                return;
            }
            __delay_us(100);
        }
        
        // Fase de apagado del PWM por software
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

/*void homing_m2() {
    DIR_A = 1;
    DIR_B = 0;
    int pulsos_on = 10;   
    int pulsos_off = 31; 

    while(1) {
        ENABLE_M2 = 1; // Usamos el ENABLE_M2 para mover el segundo motor
        for(int i = 0; i < pulsos_on; i++) {
            if(SENS2_M2 == 0) { 
                frenado_dinamico_m2();
                posicion_m2 = 1;
                return;
            }
            __delay_us(100);
        }
        
        ENABLE_M2 = 0;
        for(int i = 0; i < pulsos_off; i++) {
            if(SENS2_M2 == 0) { 
                frenado_dinamico_m2();
                posicion_m2 = 1;
                return;
            }
            __delay_us(100);
        }
    }
}                                   */

// ========================================================
// 6. MÁQUINA DE ESTADO DE POSICIONAMIENTO M1 (Actividades 2 y 4)
// Controla el avance por conteo de ranuras e integra sistemas 
// de autocorrección mecánica y seguro contra atascos (Watchdog).
// ========================================================
void ir_a_posicion_m1(int posicion_destino) {
    // 1. Calcula la distancia a recorrer en pasos de contacto
    int pasos_a_dar = posicion_destino - posicion_m1;
    
    if (pasos_a_dar == 0) return; // Se aborta si el motor ya está en el destino solicitado

    // 2. Establece el sentido de giro físico
    if (pasos_a_dar > 0) {
        DIR_A = 1; // Avance numérico (ej. Posición 2 a la 5)
        DIR_B = 0;
    } else {
        DIR_A = 0; // Retroceso numérico (ej. Posición 10 a la 4)
        DIR_B = 1;
        pasos_a_dar = -pasos_a_dar; // Valor absoluto para usar como contador
    }

    // El disco físico tiene 60 muescas y 12 pasos. Es decir, 5 ranuras por canal.
    int ranuras_objetivo = pasos_a_dar * 5; 
    int ranuras_contadas = 0;
    
    // Captura inicial de la línea para detección de flancos en tiempo real
    int estado_anterior = SENS1_M1; 
    
    // Variables para control de inercia y velocidad (PWM)
    int pulsos_on = 10;   
    int pulsos_off = 31; 
    int ciclo_pwm = 0;

    // Watchdog Timer (WDT) por software para evitar quemar el puente H
    unsigned int timeout_contador = 0; 
    const unsigned int TIMEOUT_MAX = 30000; // Aproximadamente 5 segundos basado en retardos

    while(1) { 
        // --- 6.1 WATCHDOG / SEGURO CONTRA ATASCOS ---
        timeout_contador++;
        if (timeout_contador >= TIMEOUT_MAX) {
            frenado_dinamico_m1(); // El disco se bloqueó físicamente. Frena por emergencia.
            while(1) {
                // Estado de trampa infinita. El sistema entra en falla crítica.
                // En la Fase 3, aquí se enviará la trama UART (0xEE) al maestro ESP32.
                __delay_ms(100); 
            }
        }

        // --- 6.2 GENERADOR DE VELOCIDAD LENTA (PWM) ---
        ciclo_pwm++;
        if (ciclo_pwm <= pulsos_on) ENABLE_M1 = 1;
        else if (ciclo_pwm <= (pulsos_on + pulsos_off)) ENABLE_M1 = 0;
        else ciclo_pwm = 0;

        // --- 6.3 LÓGICA DE FLANCOS Y CONTROL DE TRAYECTORIA ---
        int lectura_actual = SENS1_M1;

        if (lectura_actual != estado_anterior) { // El sensor acaba de transicionar
            
            timeout_contador = 0; // Si hay transición significa que el motor gira; reiniciamos el Watchdog
            
            if (lectura_actual == 0) { // Flanco de bajada (de madera a luz). Entramos a una muesca.
                ranuras_contadas++; 
                
                // REDUCCIÓN DINÁMICA DE VELOCIDAD DE APROXIMACIÓN
                // Al llegar a la penúltima ranura, quita potencia para entrar al destino casi resbalando.
                if (ranuras_contadas == (ranuras_objetivo - 1)) {
                    pulsos_on = 5;   // Baja el Duty Cycle a la mitad
                    pulsos_off = 36; // Compensa el tiempo off para no alterar la frecuencia base
                }
                
                // DETECCIÓN DE POSICIÓN FINAL
                if (ranuras_contadas >= ranuras_objetivo) {
                    
                    // Respalda el sentido de giro actual antes de que el frenazo lo borre
                    int dir_a_original = DIR_A;
                    int dir_b_original = DIR_B;
                    
                    frenado_dinamico_m1(); // Parada de inercia
                    
                    // --- AUTOCORRECCIÓN MECÁNICA EN LAZO CERRADO ---
                    __delay_ms(15); // Pausa de estabilización mecánica (dead-time)
                    
                    /*En este caso dejamos la funcion ya que presentabamos escenarios donde el sensor quedaba entre la madera de dos muescas cosecutivas
                     * Sin embargo es importante acalara que con la logica de reducir el duty cicle la mayoria quedan en su punto exacto
                     * Pero al ser ms el ojo humano podria estar confundiendose por lo que se deja esta parte del codigos como metodo de prevencion
                     * y asi aseguramos que el sensor optico siempre quede dentro de una muesca
                     */
                    if (SENS1_M1 == 1) { // Si el disco derrapó y el láser quedó obstruido (Madera)
                        
                        // Restaura el sentido de avance
                        DIR_A = dir_a_original;
                        DIR_B = dir_b_original;
                        
                        // Inicia ráfaga de micro-impulsos de torque hacia adelante
                        while(SENS1_M1 == 1) {
                            ENABLE_M1 = 1;
                            __delay_ms(15);  // Empujón eléctrico
                            ENABLE_M1 = 0;
                            __delay_ms(20);  // Resbalón controlado
                        }
                        
                        // El sensor encontró el hueco, clava los frenos para anclaje definitivo
                        frenado_dinamico_m1();
                    }
                    
                    // Finalización de la rutina y actualización de memoria espacial
                    posicion_m1 = posicion_destino;
                    return; 
                }
            }
            estado_anterior = lectura_actual; // Prepara la variable para el siguiente flanco
        }

        // Base de tiempo principal del sistema de control
        __delay_us(100);
    }
}

/*
// ========================================================
// 7. MÁQUINA DE ESTADO DE POSICIONAMIENTO M2
// Funciona bajo la misma lógica algorítmica de M1, pero 
// afecta exclusivamente al hardware del Canal de ENABLE_M2.
// ========================================================
void ir_a_posicion_m2(int posicion_destino) {
    int pasos_a_dar = posicion_destino - posicion_m2;
    if (pasos_a_dar == 0) return; 

    // Al estar multiplexados, usamos las MISMA líneas de dirección
    if (pasos_a_dar > 0) {
        DIR_A = 1; 
        DIR_B = 0;
    } else {
        DIR_A = 0; 
        DIR_B = 1;
        pasos_a_dar = -pasos_a_dar; 
    }

    int ranuras_objetivo = pasos_a_dar * 5; 
    int ranuras_contadas = 0;
    int estado_anterior = SENS1_M2; // Censo de M2
    
    int pulsos_on = 10;   
    int pulsos_off = 31; 
    int ciclo_pwm = 0;

    unsigned int timeout_contador = 0; 
    const unsigned int TIMEOUT_MAX = 30000; 

    while(1) { 
        timeout_contador++;
        if (timeout_contador >= TIMEOUT_MAX) {
            frenado_dinamico_m2(); 
            while(1) { __delay_ms(100); }
        }

        ciclo_pwm++;
        // Aplica el tren de pulsos SOLO al Enable del Motor 2
        if (ciclo_pwm <= pulsos_on) ENABLE_M2 = 1;
        else if (ciclo_pwm <= (pulsos_on + pulsos_off)) ENABLE_M2 = 0;
        else ciclo_pwm = 0;

        int lectura_actual = SENS1_M2; 

        if (lectura_actual != estado_anterior) { 
            timeout_contador = 0; 
            
            if (lectura_actual == 0) { 
                ranuras_contadas++; 
                
                if (ranuras_contadas == (ranuras_objetivo - 1)) {
                    pulsos_on = 5;   
                    pulsos_off = 36; 
                }
                
                if (ranuras_contadas >= ranuras_objetivo) {
                    int dir_a_original = DIR_A;
                    int dir_b_original = DIR_B;
                    
                    frenado_dinamico_m2(); 
                    __delay_ms(15); 
                    
                    if (SENS1_M2 == 1) { 
                        DIR_A = dir_a_original;
                        DIR_B = dir_b_original;
                        
                        while(SENS1_M2 == 1) {
                            ENABLE_M2 = 1;
                            __delay_ms(15);  
                            ENABLE_M2 = 0;
                            __delay_ms(20);  
                        }
                        frenado_dinamico_m2();
                    }
                    
                    posicion_m2 = posicion_destino;
                    return; 
                }
            }
            estado_anterior = lectura_actual; 
        }
        __delay_us(100);
    }
}                                   */

// ========================================================
// 8. BUCLE PRINCIPAL (MAIN) - ENSAYOS DE REPETIBILIDAD
// Ejecuta el "Checkpoint 2.3": 50 ciclos continuos de selección
// de canales aleatorios para comprobar el estrés mecánico.
// ========================================================
void main(void) {
    inicializar();
    __delay_ms(2000); // Tiempo muerto de seguridad post-encendido
    
    // 1. Alineación inicial: Ambos motores buscan de forma autónoma el cero de fábrica
    homing_m1(); 
    //homing_m2();
    __delay_ms(2000); // Pausa visual para verificar alineación en el canal 1/13
    
    // 2. Prueba de estrés de 50 ciclos (Aleatoriedad determinista para validación industrial)
    for (int ciclo = 1; ciclo <= 50; ciclo++) {
        
        // rand() % 12 genera números aleatorios entre 0 y 11. Se suma 1 para rango de canales (1 al 12)
        int pos_destino_m1 = (rand() % 12) + 1;
        //int pos_destino_m2 = (rand() % 12) + 1;
        
        // Ejecución secuencial (No simultánea, por exigencia de corriente del L293D)
        ir_a_posicion_m1(pos_destino_m1);
        __delay_ms(500); 
        
        //ir_a_posicion_m2(pos_destino_m2);
        
        // Pausa de 3 segundos obligatoria para validación ocular con marcas o microscopio
        //__delay_ms(3000); 
    }

    // 3. Regreso triunfal a posición inicial tras superar las pruebas
    ir_a_posicion_m1(1);
    //ir_a_posicion_m2(1);
    
    // Congela el procesador de forma segura
    while(1) {
        __delay_ms(100);
    }
}