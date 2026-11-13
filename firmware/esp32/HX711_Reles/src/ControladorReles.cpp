/**
 * @file ControladorReles.cpp
 * @brief Implementación del secuenciador de relés K1..K5 con tiempos muertos.
 */

#include "ControladorReles.h"

ControladorReles::ControladorReles()
    : _modoActual(MODO_DEFORMACION),
      _tipoSensorActual(SENSOR_TIPO_1) {

    pinMode(RELE_HABILITACION, OUTPUT);
    pinMode(RELE_ALIMENTACION, OUTPUT);
    pinMode(RELE_MODO_BIT0,    OUTPUT);
    pinMode(RELE_MODO_BIT1,    OUTPUT);
    pinMode(RELE_TIPO_SENSOR,  OUTPUT);

    estadoSeguro();
}

void ControladorReles::_habilitarMedicion() {
    digitalWrite(RELE_HABILITACION, HIGH);
}

void ControladorReles::_deshabilitarMedicion() {
    digitalWrite(RELE_HABILITACION, LOW);
}

void ControladorReles::_energizarPuente() {
    digitalWrite(RELE_ALIMENTACION, HIGH);
}

void ControladorReles::_desenergizarPuente() {
    digitalWrite(RELE_ALIMENTACION, LOW);
}

void ControladorReles::_setTipoSensor(TipoSensor tipo) {
    digitalWrite(RELE_TIPO_SENSOR, (tipo == SENSOR_TIPO_2) ? HIGH : LOW);
    _tipoSensorActual = tipo;
}

void ControladorReles::_setModo(ModoMedicion modo) {
    if (modo == MODO_DEFORMACION) {
        // En hardware físico: RELE 3 = 0, RELE 4 = 0 (contactos NC activos para el puente)
        digitalWrite(RELE_MODO_BIT1, LOW);
        digitalWrite(RELE_MODO_BIT0, LOW);
    } else {
        // RELE 3 = 1, RELE 4 = 1
        digitalWrite(RELE_MODO_BIT1, HIGH);
        digitalWrite(RELE_MODO_BIT0, HIGH);
    }
    _modoActual = modo;
}

void ControladorReles::estadoSeguro() {
    _deshabilitarMedicion();
    _desenergizarPuente();
    _setTipoSensor(SENSOR_TIPO_1);
    _setModo(MODO_DEFORMACION);

    Serial.println("[RELES] Estado seguro configurado.");
}

void ControladorReles::iniciarMedicionDeformacion(TipoSensor tipo) {
    _deshabilitarMedicion();
    _desenergizarPuente();
    delay(DEAD_TIME_MS);

    _setTipoSensor(tipo);
    _setModo(MODO_DEFORMACION);
    delay(DEAD_TIME_MS);

    _energizarPuente();
    delay(DEAD_TIME_MS);
    _habilitarMedicion();
    delay(DEAD_TIME_MS);

    Serial.print("[RELES] Modo DEFORMACION activo. Sensor: ");
    Serial.println(tipo == SENSOR_TIPO_1 ? "350Ω" : "120Ω");
}

void ControladorReles::iniciarMedicionTemperatura(TipoSensor tipo) {
    _deshabilitarMedicion();
    _desenergizarPuente();
    delay(DEAD_TIME_MS);

    _setTipoSensor(tipo);
    _setModo(MODO_TEMPERATURA);
    delay(DEAD_TIME_MS);

    _energizarPuente();
    delay(DEAD_TIME_MS);
    _habilitarMedicion();
    delay(DEAD_TIME_MS);

    Serial.print("[RELES] Modo TEMPERATURA activo. Sensor: ");
    Serial.println(tipo == SENSOR_TIPO_1 ? "350Ω" : "120Ω");
}

void ControladorReles::detenerMedicion() {
    _deshabilitarMedicion();
    _desenergizarPuente();
    Serial.println("[RELES] Medición detenida (puente en reposo).");
}

