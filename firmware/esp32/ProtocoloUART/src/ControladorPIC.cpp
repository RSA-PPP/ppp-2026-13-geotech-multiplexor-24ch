#include "ControladorPIC.h"

void ControladorPIC::iniciar() {
    // UART0 queda reservado para los mensajes de depuración por USB.
    Serial.begin(115200);
    Serial2.begin(9600, SERIAL_8N1, rxPin, txPin);
    delay(500);
}

bool ControladorPIC::enviarComando(uint8_t motor_id, uint8_t cmd, uint8_t param) {
    uint8_t checksum = motor_id ^ cmd ^ param;
    uint8_t trama[6] = {HEADER, motor_id, cmd, param, checksum, TAIL};

    while (Serial2.available()) {
        Serial2.read();
    }

    for (int intento = 1; intento <= 3; intento++) {
        Serial2.write(trama, sizeof(trama));
        Serial2.flush();
        Serial.print(" Intento ");
        Serial.print(intento);
        Serial.println("...");

        unsigned long inicioTiempo = millis();
        uint8_t respuesta[6];
        size_t recibidos = 0;

        while (millis() - inicioTiempo < 10000) {
            while (Serial2.available() && recibidos < sizeof(respuesta)) {
                respuesta[recibidos++] = static_cast<uint8_t>(Serial2.read());
            }

            if (recibidos == sizeof(respuesta)) {
                const uint8_t checksumRespuesta =
                    respuesta[1] ^ respuesta[2] ^ respuesta[3];
                const bool valida =
                    respuesta[0] == RESPONSE_HEADER &&
                    respuesta[4] == checksumRespuesta &&
                    respuesta[5] == TAIL;

                if (valida) {
                    // Extraer variables reales de la trama de respuesta
                    ultimo_estado = respuesta[2];
                    ultima_posicion = respuesta[3];

                    Serial.print("Trama valida. Estado: 0x");
                    Serial.print(ultimo_estado, HEX);
                    Serial.print(" | Posicion: ");
                    Serial.println(ultima_posicion);

                    if (ultimo_estado == 0xEE) {
                        Serial.println("¡ALERTA! Atasco mecánico detectado (0xEE).");
                    } else if (ultimo_estado == 0x00) {
                        Serial.println("Movimiento completado (READY).");
                    }
                    return true;
                }

                Serial.println("Respuesta invalida recibida del PIC.");
                break;
            }
            delay(1);
        }

        Serial.println("Timeout o trama invalida. Reintentando...");
    }

    Serial.println("Timeout o Error.");
    return false;
}