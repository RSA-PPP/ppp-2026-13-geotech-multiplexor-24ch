#ifndef CONTROLADOR_PIC_H
#define CONTROLADOR_PIC_H

#include <Arduino.h>

class ControladorPIC {
private:
    // Pines UART para el PIC16F628A ruteados en la placa de acople (STELLARIS1)
    // Pin 24 (IO26) = Pista 'TX' de motores (Salida 5V del PIC -> Entrada RX del ESP32 con resistencia)
    // Pin 25 (IO27) = Pista 'RX' de motores (Salida TX del ESP32 3.3V -> Entrada RX del PIC)
    static constexpr int rxPin = 26;  // Recibe datos desde el TX del PIC (RB2)
    static constexpr int txPin = 27;  // Transmite comandos hacia el RX del PIC (RB1)

    // Delimitadores del protocolo
    static constexpr uint8_t HEADER = 0xAA;
    static constexpr uint8_t RESPONSE_HEADER = 0xBB;
    static constexpr uint8_t TAIL = 0x55;

public:
    // Variables públicas para leer la respuesta (Fase 3)
    uint8_t ultimo_estado = 0x00;
    uint8_t ultima_posicion = 0;

    // Declaración de las funciones
    void iniciar();
    bool enviarComando(uint8_t motor_id, uint8_t cmd, uint8_t param);
};

#endif
