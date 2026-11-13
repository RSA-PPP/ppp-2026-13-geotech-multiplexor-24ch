/**
 * @file DataloggerSD.h
 * @brief Controlador del módulo MicroSD por bus SPI nativo (VSPI) en FAT32
 */

#ifndef DATALOGGER_SD_H
#define DATALOGGER_SD_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

class DataloggerSD {
private:
    uint8_t  _csPin;
    uint8_t  _sckPin;
    uint8_t  _misoPin;
    uint8_t  _mosiPin;
    bool     _disponible;
    uint32_t _numRegistros; // Contador en memoria de registros guardados
    SPIClass _spiSD;

    void contarRegistrosIniciales();

public:
    DataloggerSD(uint8_t csPin = 5, uint8_t sckPin = 18, uint8_t misoPin = 19, uint8_t mosiPin = 23);
    
    bool iniciar();
    bool estaDisponible() const { return _disponible; }

    bool registrar(uint32_t timestamp, const char* fecha, const char* hora,
                   uint8_t canal, const char* modo, int32_t rawAdc,
                   float valorIng, const char* estado);

    // Retorna espacio libre en SD (MB) y número exacto de registros almacenados
    void obtenerEstado(uint64_t &espacioLibreMB, uint32_t &numRegistros);
};

#endif // DATALOGGER_SD_H
