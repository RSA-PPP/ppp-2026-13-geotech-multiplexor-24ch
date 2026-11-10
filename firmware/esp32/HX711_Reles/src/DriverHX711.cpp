/**
 * @file DriverHX711.cpp
 * @brief Implementación del driver HX711 para ESP32 en C++/Arduino.
 */

#include "DriverHX711.h"

DriverHX711::DriverHX711(uint8_t pinDOUT, uint8_t pinSCK)
    : _pinDOUT(pinDOUT), _pinSCK(pinSCK),
      _ultimoCrudo(0), _offset(0), _escala(1.0f) {
}

bool DriverHX711::iniciar() {
    pinMode(_pinDOUT, INPUT_PULLUP);
    pinMode(_pinSCK, OUTPUT);

    // Secuencia de Reset / Wake-Up según datasheet del HX711:
    // 1. Pulso HIGH > 60 µs para forzar reset / power-down
    digitalWrite(_pinSCK, HIGH);
    delayMicroseconds(100);

    // 2. Regresar a LOW para despertar el chip y arrancar el conversor
    digitalWrite(_pinSCK, LOW);
    delay(400); // El oscilador a 10 SPS tarda ~100-400 ms en la primera muestra

    // Diagnóstico del estado del pin DOUT tras despertar
    int estadoInicial = digitalRead(_pinDOUT);
    Serial.print("[HX711] Estado pin DOUT (GPIO ");
    Serial.print(_pinDOUT);
    Serial.print("): ");
    Serial.println(estadoInicial == HIGH ? "HIGH (Esperando conversion)" : "LOW (Dato listo)");

    // Esperar a que el HX711 baje DOUT a LOW con timeout de 3 segundos
    uint32_t inicio = millis();
    while (digitalRead(_pinDOUT) == HIGH) {
        if (millis() - inicio > 3000) {
            Serial.println("[HX711] ERROR: Timeout esperando dato inicial tras Wake-Up.");
            return false;
        }
        yield();
    }

    // Lectura de descarte para fijar Ganancia 128 (Canal A)
    int32_t descarte = leerCrudo();
    Serial.print("[HX711] Lectura descarte arranque: ");
    Serial.println(descarte);

    Serial.println("[HX711] Inicializado OK. Canal A, Ganancia 128.");
    return true;
}

bool DriverHX711::haydato() {
    return (digitalRead(_pinDOUT) == LOW);
}

uint8_t DriverHX711::_leerBit() {
    digitalWrite(_pinSCK, HIGH);
    delayMicroseconds(HX711_PULSO_US);

    uint8_t bit = digitalRead(_pinDOUT);

    digitalWrite(_pinSCK, LOW);
    delayMicroseconds(HX711_PULSO_US);

    return bit;
}

int32_t DriverHX711::leerCrudo(uint32_t timeout_ms) {
    uint32_t inicio = millis();
    while (!haydato()) {
        if (millis() - inicio > timeout_ms) {
            Serial.println("[HX711] TIMEOUT esperando dato.");
            return INT32_MIN;
        }
        yield(); // Espera cooperativa no bloqueante para el RTOS
    }

    int32_t valor = 0;

    // Sección crítica: evita que una interrupción deje SCK en HIGH > 60us
    noInterrupts();
    for (int i = 0; i < 24; i++) {
        valor = (valor << 1) | _leerBit();
    }
    // 25to pulso: configura Canal A, Ganancia 128 para la siguiente lectura
    _leerBit();
    interrupts();

    // Extensión de signo de 24 bits (Complemento a 2)
    if (valor & 0x800000) {
        valor -= 0x1000000;
    }

    _ultimoCrudo = valor;
    return valor;
}

void DriverHX711::_ordenarArreglo(int32_t* arr, uint8_t n) {
    for (uint8_t i = 0; i < n - 1; i++) {
        for (uint8_t j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                int32_t tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

int32_t DriverHX711::leerMediana(uint8_t numMuestras, uint8_t descartarPrimeras) {
    // 1. Descarte de muestras post-conmutación de relés
    for (uint8_t i = 0; i < descartarPrimeras; i++) {
        if (leerCrudo() == INT32_MIN) {
            return INT32_MIN;
        }
    }

    // 2. Captura de ventana de muestras consecutivas
    int32_t muestras[HX711_MUESTRAS_MEDIANA];
    uint8_t validas = 0;

    for (uint8_t i = 0; i < numMuestras && i < HX711_MUESTRAS_MEDIANA; i++) {
        int32_t val = leerCrudo();
        if (val != INT32_MIN) {
            muestras[validas++] = val;
        }
    }

    if (validas == 0) {
        return INT32_MIN;
    }

    // 3. Ordenamiento y retorno de mediana
    _ordenarArreglo(muestras, validas);
    return muestras[validas / 2];
}

float DriverHX711::leerUnidades(uint8_t numMuestras, uint8_t descartarPrimeras) {
    int32_t mediana = leerMediana(numMuestras, descartarPrimeras);
    if (mediana == INT32_MIN) {
        return NAN;
    }
    return (float)(mediana - _offset) / _escala;
}

bool DriverHX711::tarar(uint8_t muestras) {
    Serial.println("[HX711] Calculando tara (offset)...");

    int64_t acumulado = 0;
    uint8_t validas = 0;

    for (uint8_t i = 0; i < muestras; i++) {
        int32_t val = leerCrudo();
        if (val != INT32_MIN) {
            acumulado += val;
            validas++;
        }
    }

    if (validas == 0) return false;

    _offset = (int32_t)(acumulado / validas);
    Serial.print("[HX711] Offset de tara fijado: ");
    Serial.println(_offset);
    return true;
}

void DriverHX711::setEscala(float escala) {
    if (escala != 0.0f) {
        _escala = escala;
    }
}

void DriverHX711::apagar() {
    digitalWrite(_pinSCK, LOW);
    delayMicroseconds(1);
    digitalWrite(_pinSCK, HIGH);
    delayMicroseconds(70);
    Serial.println("[HX711] Modo bajo consumo activado.");
}

void DriverHX711::encender() {
    digitalWrite(_pinSCK, LOW);
    delay(500);
    Serial.println("[HX711] Despertado del modo bajo consumo.");
}
