/**
 * @file ControladorReles.h
 * @brief Secuenciador de relés K1..K5 para el puente de Wheatstone.
 *
 * Proyecto: RSA-PPP-2026-13 — Multiplexor Geotécnico 24 Canales
 * Fase 4: Adquisición analógica y secuenciamiento de relés.
 *
 * Mapeo de pines (tarjeta acople_esp32):
 *   GPIO 13 → Habilitación medición (K1)
 *   GPIO 14 → Alimentación puente (3.3V excitación) (K2)
 *   GPIO 16 → Modo Bit 0 (K3)
 *   GPIO 17 → Modo Bit 1 (K4)
 *   GPIO 25 → Tipo sensor (K5)
 */

#ifndef CONTROLADOR_RELES_H
#define CONTROLADOR_RELES_H

#include <Arduino.h>

#define RELE_HABILITACION   13   // K1: Habilitacion medicion (IO13)
#define RELE_ALIMENTACION   14   // K2: Alimentacion puente (IO14)
#define RELE_MODO_BIT0      16   // K3: Modo Bit 0 (IO16)
#define RELE_MODO_BIT1      17   // K4: Modo Bit 1 (IO17)
#define RELE_TIPO_SENSOR    25   // K5: Tipo de sensor (IO25)

#define DEAD_TIME_MS        20

enum ModoMedicion {
    MODO_DEFORMACION  = 0,   // GPIO17=0, GPIO16=0 → deformación µε
    MODO_TEMPERATURA  = 1    // GPIO17=1, GPIO16=1 → temperatura °C
};

enum TipoSensor {
    SENSOR_TIPO_1 = 0,  // Galga de 350Ω
    SENSOR_TIPO_2 = 1   // Galga de 120Ω
};

class ControladorReles {
public:
    ControladorReles();

    void estadoSeguro();
    void iniciarMedicionDeformacion(TipoSensor tipo = SENSOR_TIPO_1);
    void iniciarMedicionTemperatura(TipoSensor tipo = SENSOR_TIPO_1);
    void detenerMedicion();

    ModoMedicion getModoActual() const { return _modoActual; }
    TipoSensor getTipoSensorActual() const { return _tipoSensorActual; }

private:
    ModoMedicion _modoActual;
    TipoSensor   _tipoSensorActual;

    void _habilitarMedicion();
    void _deshabilitarMedicion();
    void _energizarPuente();
    void _desenergizarPuente();
    void _setModo(ModoMedicion modo);
    void _setTipoSensor(TipoSensor tipo);
};

#endif // CONTROLADOR_RELES_H

