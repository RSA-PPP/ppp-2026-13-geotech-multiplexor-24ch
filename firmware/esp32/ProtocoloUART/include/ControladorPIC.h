#ifndef CONTROLADOR_PIC_H
#define CONTROLADOR_PIC_H

#include <Arduino.h>

class ControladorPIC {
private:
    // Pines UART2  (RX_2 y TX_2 en la placa de acople)
    static constexpr int rxPin = 18;
    static constexpr int txPin = 17;

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