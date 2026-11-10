/**
 * @file main.cpp
 * @brief Programa de prueba para Fase 4: HX711 + Relés K1..K5
 *
 * Proyecto: RSA-PPP-2026-13 — Multiplexor Geotécnico 24 Canales
 * Hardware: Placa acople_esp32 + ESP32 Dev Module (WROOM)
 */

#include <Arduino.h>
#include "DriverHX711.h"
#include "ControladorReles.h"

// Pines asignados en placa de acople para ESP32 clasico
#define PIN_HX711_DOUT   4    // GPIO 4  (IO4 / DT)
#define PIN_HX711_SCK    33   // GPIO 33 (A5 / SCK_)

// Factores de calibración (Actividad 5: actualizado tras regresión lineal experimental R² = 0.99978)
#define ESCALA_DEFORMACION   193.70f   // Cuentas por gramo (193.70 LSB/g)
#define ESCALA_TEMPERATURA   1.0f      // Cuentas por cada °C

DriverHX711       hx711(PIN_HX711_DOUT, PIN_HX711_SCK);
ControladorReles  reles;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n======================================");
    Serial.println("  RSA-PPP-2026-13 — PROYECTO HX711_Reles");
    Serial.println("  Fase 4: Driver HX711 + Relés K1..K5");
    Serial.println("======================================\n");

    // 1. Relés en estado seguro
    Serial.println("[SETUP] Asegurando relés...");
    reles.estadoSeguro();

    // 2. Inicializar el convertidor HX711
    Serial.println("[SETUP] Inicializando HX711...");
    if (!hx711.iniciar()) {
        Serial.println("[SETUP] ¡ALERTA! El HX711 no responde.");
        Serial.println("  -> Revisa conexión en GPIO 4 (DT) y GPIO 33 (SCK).");
        Serial.println("  -> Revisa alimentación de 5V y GND.");
        while (true) {
            delay(1000);
        }
    }

    // 3. Tarar / calibrar offset en reposo
    Serial.println("[SETUP] Calculando tara de offset inicial...");
    reles.iniciarMedicionDeformacion(SENSOR_TIPO_1);
    hx711.tarar(15);
    reles.detenerMedicion();

    Serial.println("[SETUP] Inicialización completada. Iniciando ciclo...\n");
}

void loop() {
    static uint32_t ciclo = 0;
    ciclo++;

    Serial.print("\n=== Ciclo #");
    Serial.print(ciclo);
    Serial.println(" ==============================");

    // -------------------------------------------------------------
    // 1. Medición de Deformación Mecánica (Modo 00)
    // -------------------------------------------------------------
    Serial.println(">>> Modo DEFORMACION (µε)");
    reles.iniciarMedicionDeformacion(SENSOR_TIPO_1);

    // Adquisición por mediana (descarta 2 transitorios iniciales y toma 5 muestras)
    int32_t rawDeformacion = hx711.leerMediana(5, 2);
    int32_t cuentasDeformacion = (rawDeformacion != INT32_MIN) 
                                 ? (rawDeformacion - hx711.getOffset()) 
                                 : INT32_MIN;
    float valorDeformacion = (cuentasDeformacion != INT32_MIN)
                             ? ((float)cuentasDeformacion / ESCALA_DEFORMACION)
                             : NAN;

    reles.detenerMedicion();

    // -------------------------------------------------------------
    // 2. Medición de Temperatura Interna (Modo 11)
    // -------------------------------------------------------------
    Serial.println(">>> Modo TEMPERATURA (°C)");
    reles.iniciarMedicionTemperatura(SENSOR_TIPO_1);

    int32_t rawTemperatura = hx711.leerMediana(5, 2);
    int32_t cuentasTemperatura = (rawTemperatura != INT32_MIN) 
                                 ? (rawTemperatura - hx711.getOffset()) 
                                 : INT32_MIN;
    float valorTemperatura = (cuentasTemperatura != INT32_MIN)
                             ? ((float)cuentasTemperatura / ESCALA_TEMPERATURA)
                             : NAN;

    reles.detenerMedicion();

    // -------------------------------------------------------------
    // 3. Reporte de Datos
    // -------------------------------------------------------------
    Serial.println("--- Resultados ---");
    if (cuentasDeformacion != INT32_MIN) {
        Serial.print("  Deformacion : ");
        Serial.print(valorDeformacion, 2);
        Serial.print(" µε | Cuentas netas: ");
        Serial.print(cuentasDeformacion);
        Serial.print(" | (RAW: ");
        Serial.print(rawDeformacion);
        Serial.println(")");
    } else {
        Serial.println("  Deformacion : ERROR de lectura");
    }

    if (cuentasTemperatura != INT32_MIN) {
        Serial.print("  Temperatura : ");
        Serial.print(valorTemperatura, 2);
        Serial.print(" °C | Cuentas netas: ");
        Serial.print(cuentasTemperatura);
        Serial.print(" | (RAW: ");
        Serial.print(rawTemperatura);
        Serial.println(")");
    } else {
        Serial.println("  Temperatura : ERROR de lectura");
    }
    Serial.println("-------------------\n");

    delay(3000);
}