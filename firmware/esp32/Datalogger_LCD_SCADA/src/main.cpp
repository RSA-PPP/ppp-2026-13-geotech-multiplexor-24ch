/**
 * @file main.cpp
 * @brief Orquestador principal de la Fase 5: Datalogger MicroSD, Pantalla LCD y Comandos SCADA
 * 
 * Proyecto: RSA-PPP-2026-13 — Multiplexor Geotécnico de 24 Canales
 * Autor: Walter Calderón / RSA - Universidad de Cuenca
 */

#include <Arduino.h>
#include "DataloggerSD.h"
#include "InterfazLCD.h"
#include "ComandosSCADA.h"

// Instanciación de módulos
DataloggerSD  datalogger(5);                         // CS en GPIO 5 (VSPI nativo)
InterfazLCD   pantalla(0x27);                        // LCD I2C en 0x27 (SDA: GPIO 21, SCL: GPIO 22)
ComandosSCADA scada(Serial, datalogger, pantalla);   // Intérprete por Serial USB (115200 bps)

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=======================================================");
    Serial.println("  RSA-PPP-2026-13 — SISTEMA MULTIPLEXOR GEOTECNICO 24CH");
    Serial.println("  FASE 5: Datalogger MicroSD, Pantalla LCD y SCADA ASCII");
    Serial.println("=======================================================\n");

    // 1. Inicializar Pantalla LCD (I2C)
    Serial.print("[INIT] Inicializando Pantalla LCD (I2C)... ");
    if (pantalla.iniciar()) {
        Serial.println("OK");
        pantalla.mostrarBienvenida();
    } else {
        Serial.println("FALLO (Verifica conexiones SDA=21, SCL=22 o dirección I2C)");
    }

    // 2. Inicializar Tarjeta MicroSD (SPI)
    Serial.print("[INIT] Inicializando Tarjeta MicroSD (FAT32)... ");
    if (datalogger.iniciar()) {
        Serial.println("OK");
        pantalla.mostrarEstado("SD OK - LISTO");
    } else {
        Serial.println("FALLO (Verifica tarjeta formateada en FAT32 y pines SPI: CS=5, SCK=18, MOSI=23, MISO=19)");
        pantalla.mostrarError("ERR: SD NO MONTA");
    }

    Serial.println("\n[SISTEMA LISTO] Esperando comandos serie (115200 bps):");
    Serial.println("  - READ:CH <1..24>");
    Serial.println("  - SCAN:ALL");
    Serial.println("  - CAL:ZERO");
    Serial.println("  - LOG:STATUS\n");
}

void loop() {
    // Escucha de comandos serie en tiempo real (no bloqueante)
    scada.escuchar();
}
