/**
 * @file InterfazLCD.h
 * @brief Controlador para pantalla LCD Alfanumérica 16x2 / 20x4 vía I2C sin parpadeo (flicker-free)
 * 
 * Proyecto: RSA-PPP-2026-13 — Multiplexor Geotécnico de 24 Canales
 * Pines asignados:
 *   SDA: GPIO 21
 *   SCL: GPIO 22
 */

#ifndef INTERFAZ_LCD_H
#define INTERFAZ_LCD_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class InterfazLCD {
private:
    uint8_t _i2cAddr;
    uint8_t _sdaPin;
    uint8_t _sclPin;
    LiquidCrystal_I2C _lcd;
    bool _iniciado;

public:
    InterfazLCD(uint8_t i2cAddr = 0x27, uint8_t sdaPin = 21, uint8_t sclPin = 22);

    bool iniciar();
    void mostrarBienvenida();
    void actualizarMedicion(uint8_t canal, float microstrain, float temperatura);
    void mostrarEstado(const char* mensaje);
    void mostrarError(const char* mensaje);
};

#endif // INTERFAZ_LCD_H
