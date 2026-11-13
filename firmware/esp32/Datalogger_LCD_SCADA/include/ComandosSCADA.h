/**
 * @file ComandosSCADA.h
 * @brief Intérprete del protocolo serie RS-232 / SCADA ASCII
 * 
 * Proyecto: RSA-PPP-2026-13 — Multiplexor Geotécnico de 24 Canales
 * Comandos soportados:
 *   - READ:CH <1..24>
 *   - SCAN:ALL
 *   - CAL:ZERO
 *   - LOG:STATUS
 */

#ifndef COMANDOS_SCADA_H
#define COMANDOS_SCADA_H

#include <Arduino.h>
#include "DataloggerSD.h"
#include "InterfazLCD.h"

class ComandosSCADA {
private:
    Stream&       _puerto;
    DataloggerSD& _sd;
    InterfazLCD&  _lcd;
    String        _buffer;

public:
    ComandosSCADA(Stream& puerto, DataloggerSD& sd, InterfazLCD& lcd);

    void escuchar();
    void procesar(String cmd);
};

#endif // COMANDOS_SCADA_H
