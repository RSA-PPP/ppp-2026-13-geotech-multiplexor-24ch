/**
 * @file DriverHX711.h
 * @brief Driver de bajo nivel para el ADC diferencial HX711 de 24 bits.
 *
 * Proyecto: RSA-PPP-2026-13 — Multiplexor Geotécnico 24 Canales
 * Fase 4: Adquisición analógica y secuenciamiento de relés.
 *
 * Pines asignados en placa acople_esp32:
 *   DOUT   → GPIO 4   (va a IO4 / DT de la placa, pin 18 STELLARIS1)
 *   PD_SCK → GPIO 33  (va a A5 / SCK_ de la placa, pin 10 STELLARIS1)
 */

#ifndef DRIVER_HX711_H
#define DRIVER_HX711_H

#include <Arduino.h>

#define HX711_MUESTRAS_MEDIANA   5
#define HX711_TIMEOUT_MS         1500
// Retardo entre pulsos de reloj (µs) — aumentado a 5µs para máxima compatibilidad
#define HX711_PULSO_US           5

class DriverHX711 {
public:
    DriverHX711(uint8_t pinDOUT, uint8_t pinSCK);

    bool iniciar();
    bool haydato();
    int32_t leerCrudo(uint32_t timeout_ms = HX711_TIMEOUT_MS);
    int32_t leerMediana(uint8_t numMuestras = HX711_MUESTRAS_MEDIANA, uint8_t descartarPrimeras = 2);
    float   leerUnidades(uint8_t numMuestras = HX711_MUESTRAS_MEDIANA, uint8_t descartarPrimeras = 2);

    void apagar();
    void encender();
    bool tarar(uint8_t muestras = 20);

    void    setEscala(float escala);
    float   getEscala() const { return _escala; }
    void    setOffset(int32_t offset) { _offset = offset; }
    int32_t getOffset() const { return _offset; }
    int32_t getUltimoCrudo() const { return _ultimoCrudo; }

private:
    uint8_t  _pinDOUT;
    uint8_t  _pinSCK;
    int32_t  _ultimoCrudo;
    int32_t  _offset;
    float    _escala;

    uint8_t _leerBit();
    void _ordenarArreglo(int32_t* arr, uint8_t n);
};

#endif // DRIVER_HX711_H