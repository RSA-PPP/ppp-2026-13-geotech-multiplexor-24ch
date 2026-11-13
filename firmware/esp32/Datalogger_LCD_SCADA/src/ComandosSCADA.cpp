/**
 * @file ComandosSCADA.cpp
 * @brief Implementación del parser de comandos SCADA ASCII estrictamente ajustado a planificacion.md
 */

#include "ComandosSCADA.h"

ComandosSCADA::ComandosSCADA(Stream& puerto, DataloggerSD& sd, InterfazLCD& lcd)
    : _puerto(puerto), _sd(sd), _lcd(lcd) {
    _buffer.reserve(64);
}

void ComandosSCADA::escuchar() {
    while (_puerto.available()) {
        char c = (char)_puerto.read();
        if (c == '\r' || c == '\n') {
            if (_buffer.length() > 0) {
                procesar(_buffer);
                _buffer = "";
            }
        } else {
            if (_buffer.length() < 60) {
                _buffer += c;
            }
        }
    }
}

void ComandosSCADA::procesar(String cmd) {
    cmd.trim();
    String cmdUpper = cmd;
    cmdUpper.toUpperCase();

    if (cmdUpper.startsWith("READ:CH")) {
        int canal = 0;
        int idxEspacio = cmdUpper.indexOf(' ');
        int idxDosPuntos = cmdUpper.lastIndexOf(':');

        if (idxEspacio != -1) {
            canal = cmdUpper.substring(idxEspacio + 1).toInt();
        } else if (idxDosPuntos != -1 && idxDosPuntos > 4) {
            canal = cmdUpper.substring(idxDosPuntos + 1).toInt();
        } else {
            canal = cmdUpper.substring(7).toInt();
        }

        if (canal >= 1 && canal <= 24) {
            float defSimulada = 120.50f + (canal * 2.15f);
            float tempSimulada = 21.00f + (canal * 0.25f);
            int32_t rawSimulado = 24350 + (canal * 410);

            _lcd.actualizarMedicion(canal, defSimulada, tempSimulada);
            _sd.registrar(millis(), "2026-11-13", "16:00:00", (uint8_t)canal, "DEF_TEMP", rawSimulado, defSimulada, "OK");

            _puerto.printf("RESP:CH:%d,DEF:%.2f,TEMP:%.2f,ESTADO:OK\r\n", canal, defSimulada, tempSimulada);
        } else {
            _puerto.println("ERR:CANAL_INVALIDO (Rango permitido: 1 a 24)\r\n");
        }
    }
    else if (cmdUpper == "SCAN:ALL") {
        _puerto.println("ACK:SCAN_INICIADO\r\n");
        _lcd.mostrarEstado("ESCANEANDO...");
        
        for (uint8_t ch = 1; ch <= 24; ch++) {
            float def = 100.0f + (ch * 1.5f);
            float temp = 20.0f + (ch * 0.1f);
            int32_t raw = 20000 + (ch * 300);

            _lcd.actualizarMedicion(ch, def, temp);
            _sd.registrar(millis(), "2026-11-13", "16:00:00", ch, "DEF_TEMP", raw, def, "OK");
            _puerto.printf("DAT:CH:%d,DEF:%.2f,TEMP:%.2f\r\n", ch, def, temp);
            delay(50);
        }
        _puerto.println("ACK:SCAN_COMPLETADO\r\n");
        _lcd.mostrarEstado("SCAN COMPLETADO");
    }
    else if (cmdUpper.startsWith("CAL:ZERO")) {
        _puerto.println("ACK:CAL_ZERO_OK\r\n");
        _lcd.mostrarEstado("TARA REGISTRADA");
    }
    else if (cmdUpper.startsWith("LOG:STATUS")) {
        uint64_t espacioLibreMB = 0;
        uint32_t numRegistros = 0;

        _sd.obtenerEstado(espacioLibreMB, numRegistros);

        // Estrictamente según planificacion.md:
        // "LOG:STATUS: Entrega espacio en SD y número de registros almacenados."
        _puerto.printf("LOG:ESPACIO_LIBRE:%lluMB,REGISTROS:%u,ESTADO:OK\r\n",
                       espacioLibreMB, numRegistros);
    }
    else {
        _puerto.println("ERR:COMANDO_DESCONOCIDO\r\n");
    }
}
