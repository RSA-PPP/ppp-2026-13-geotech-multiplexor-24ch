/**
 * @file DataloggerSD.cpp
 * @brief Implementación del Datalogger con conteo de registros y cálculo de espacio libre
 */

#include "DataloggerSD.h"

DataloggerSD::DataloggerSD(uint8_t csPin, uint8_t sckPin, uint8_t misoPin, uint8_t mosiPin)
    : _csPin(csPin), _sckPin(sckPin), _misoPin(misoPin), _mosiPin(mosiPin),
      _disponible(false), _numRegistros(0), _spiSD(VSPI) {}

void DataloggerSD::contarRegistrosIniciales() {
    _numRegistros = 0;
    if (!SD.exists("/DATALOG.CSV")) return;

    File f = SD.open("/DATALOG.CSV", FILE_READ);
    if (!f) return;

    // Descartar la línea de cabecera si existe
    if (f.available()) {
        f.readStringUntil('\n');
    }

    // Contar las líneas de datos
    while (f.available()) {
        f.readStringUntil('\n');
        _numRegistros++;
    }
    f.close();
}

bool DataloggerSD::iniciar() {
    _spiSD.begin(_sckPin, _misoPin, _mosiPin, _csPin);

    if (!SD.begin(_csPin, _spiSD, 20000000)) {
        _disponible = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        _disponible = false;
        return false;
    }

    _disponible = true;

    // Si no existe, crear la cabecera
    if (!SD.exists("/DATALOG.CSV")) {
        File f = SD.open("/DATALOG.CSV", FILE_WRITE);
        if (f) {
            f.println("TIMESTAMP,FECHA,HORA,CANAL,MODO,RAW_ADC,VALOR_ING,ESTADO");
            f.flush();
            f.close();
        }
        _numRegistros = 0;
    } else {
        contarRegistrosIniciales();
    }

    return true;
}

bool DataloggerSD::registrar(uint32_t timestamp, const char* fecha, const char* hora,
                             uint8_t canal, const char* modo, int32_t rawAdc,
                             float valorIng, const char* estado) {
    if (!_disponible) return false;

    File f = SD.open("/DATALOG.CSV", FILE_APPEND);
    if (!f) return false;

    f.printf("%lu,%s,%s,%u,%s,%ld,%.2f,%s\n",
             timestamp, fecha, hora, canal, modo, rawAdc, valorIng, estado);
    f.flush();
    f.close();

    _numRegistros++; // Incrementar contador
    return true;
}

void DataloggerSD::obtenerEstado(uint64_t &espacioLibreMB, uint32_t &numRegistros) {
    if (!_disponible) {
        espacioLibreMB = 0;
        numRegistros = 0;
        return;
    }

    uint64_t totalBytes = SD.totalBytes();
    uint64_t usedBytes  = SD.usedBytes();
    
    // Espacio libre real en MB
    if (totalBytes >= usedBytes) {
        espacioLibreMB = (totalBytes - usedBytes) / (1024 * 1024);
    } else {
        espacioLibreMB = 0;
    }

    numRegistros = _numRegistros;
}
