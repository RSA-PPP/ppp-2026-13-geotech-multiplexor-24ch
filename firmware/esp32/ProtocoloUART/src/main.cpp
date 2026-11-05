#include <Arduino.h>
#include "ControladorPIC.h"

ControladorPIC pic;

// Función para el Hilo (Tarea FreeRTOS) de comunicación
void tareaUART(void *pvParameters) {
  vTaskDelay(4000 / portTICK_PERIOD_MS);
  Serial.println("Iniciando secuencia de movimientos de la Fase 2...");

  // 1. Ejecutar HOMING primero (Comando 0x01, Parámetro 0x00)
  Serial.println("\n---> Ejecutando HOMING inicial...");
  pic.enviarComando(0x01, 0x01, 0x00);

  // Pausa de 2 segundos para verificar visualmente que llegó a Home
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  // 2. Lista de posiciones solicitadas en orden
  uint8_t posiciones[] = {5, 10, 2, 10, 4, 8, 1};
  int num_posiciones = sizeof(posiciones) / sizeof(posiciones[0]);

  for (int i = 0; i < num_posiciones; i++) {
      Serial.print("\n---> Moviendo a Posicion: ");
      Serial.println(posiciones[i]);

      // Enviar comando GOTO (Comando 0x02, Parámetro = posición destino)
      bool exito = pic.enviarComando(0x01, 0x02, posiciones[i]);

      if (!exito) {
          Serial.println("\n========================================================");
          Serial.println(" [ALERTA] ¡Atasco mecánico detectado o falla de movimiento!");
          Serial.println(" La referencia de posición se ha descalibrado.");
          Serial.println(" 1. Inspeccione o libere manualmente el selector mecánico.");
          Serial.println(" 2. Envíe cualquier tecla o ENTER en el Monitor Serial...");
          Serial.println("========================================================");

          // Limpiar buffer de entrada Serial
          while (Serial.available()) {
              Serial.read();
          }

          // Esperar activamente a que el usuario presione una tecla en la consola
          while (!Serial.available()) {
              vTaskDelay(100 / portTICK_PERIOD_MS);
          }
          while (Serial.available()) {
              Serial.read();
          }

          Serial.println("\n---> Ejecutando HOMING obligatorio para recalibrar cero...");
          bool homingOk = pic.enviarComando(0x01, 0x01, 0x00);
          vTaskDelay(2000 / portTICK_PERIOD_MS);

          if (homingOk) {
              Serial.println("---> Cero absoluto recuperado con éxito.");
              Serial.print("---> Reintentando alcanzar la Posición: ");
              Serial.println(posiciones[i]);
              i--; // Decrementa para reintentar la misma posición en la siguiente iteración
              continue;
          } else {
              Serial.println("[ERROR CRÍTICO] Falló el Homing de recuperación. Abortando secuencia.");
              break;
          }
      }

      // Pausa de 2 segundos entre cada movimiento para apreciarlo físicamente
      vTaskDelay(2000 / portTICK_PERIOD_MS);
  }

  Serial.println("\n--- Secuencia de Fase 2 Finalizada con Exito ---");

  // Eliminar la tarea para que no se repita infinitamente.
  // El motor se quedará en la posición 1.
  vTaskDelete(NULL);
}

void setup() {
  pic.iniciar();
  Serial.println("\n--- Sistema Maestro ESP32 Iniciado ---");
  xTaskCreate(tareaUART, "HiloUART", 4096, NULL, 1, NULL);
}

void loop() {
  // El loop principal queda libre
  vTaskDelay(100 / portTICK_PERIOD_MS);
}