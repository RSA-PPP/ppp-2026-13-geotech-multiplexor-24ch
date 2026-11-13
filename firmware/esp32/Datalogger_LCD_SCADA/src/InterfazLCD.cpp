/**
 * @file InterfazLCD.cpp
 * @brief Implementación de la pantalla LCD cumpliendo textualmente con la actividad 3 de la Fase 5
 */

#include "InterfazLCD.h"

InterfazLCD::InterfazLCD(uint8_t i2cAddr, uint8_t sdaPin, uint8_t sclPin)
    : _i2cAddr(i2cAddr), _sdaPin(sdaPin), _sclPin(sclPin), _lcd(i2cAddr, 16, 2), _iniciado(false) {}

bool InterfazLCD::iniciar() {
    Wire.begin(_sdaPin, _sclPin);
    
    Wire.beginTransmission(_i2cAddr);
    if (Wire.endTransmission() != 0) {
        _lcd = LiquidCrystal_I2C(0x3F, 16, 2);
        Wire.beginTransmission(0x3F);
        if (Wire.endTransmission() != 0) {
            _iniciado = false;
            return false;
        }
    }

    _lcd.init();
    _lcd.backlight();
    _iniciado = true;
    return true;
}

void InterfazLCD::mostrarBienvenida() {
    if (!_iniciado) return;

    // Cumple con mostrar la versión de firmware en el estado del sistema
    _lcd.clear();
    _lcd.setCursor(0, 0);
    _lcd.print("RSA MULTIPLEXOR ");
    _lcd.setCursor(0, 1);
    _lcd.print("FW: v1.0 (FASE5)");
    delay(2500);
    _lcd.clear();
}

void InterfazLCD::actualizarMedicion(uint8_t canal, float microstrain, float temperatura) {
    if (!_iniciado) return;

    // Cumple con:
    // Fila 0: Canal en barrido actual y versión ("CH:[01/24]  v1.0") -> 16 caracteres
    // Fila 1: Lectura en microstrain y temperatura ("+1024uE 21.8C   ") -> 16 caracteres
    char bufFila0[17];
    char bufFila1[17];

    snprintf(bufFila0, sizeof(bufFila0), "CH:[%02u/24]  v1.0", canal);
    snprintf(bufFila1, sizeof(bufFila1), "%+5.0fuE %4.1fC  ", microstrain, temperatura);

    _lcd.setCursor(0, 0);
    _lcd.print(bufFila0);
    _lcd.setCursor(0, 1);
    _lcd.print(bufFila1);
}

void InterfazLCD::mostrarEstado(const char* mensaje) {
    if (!_iniciado) return;

    char buf[17];
    snprintf(buf, sizeof(buf), "%-16s", mensaje);
    _lcd.setCursor(0, 1);
    _lcd.print(buf);
}

void InterfazLCD::mostrarError(const char* mensaje) {
    if (!_iniciado) return;

    _lcd.setCursor(0, 0);
    _lcd.print("! ERROR SISTEMA !");
    mostrarEstado(mensaje);
}
