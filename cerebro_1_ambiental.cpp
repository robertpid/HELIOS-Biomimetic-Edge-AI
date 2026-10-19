/**
 * @file cerebro_1_ambiental.cpp
 * @brief Proyecto H.E.L.I.O.S. - CEREBRO 1: Módulo Ambiental del Traje Espacial (EVA)
 * @details Firmware para microcontrolador ESP32 dedicado exclusivamente al monitoreo ambiental.
 * Adquiere telemetría de sensores ambientales, empaqueta los datos y los transmite mediante
 * el protocolo inalámbrico ultrarrápido ESP-NOW hacia el Cerebro 3 (Nodo Central).
 * 
 * Principios de Clean Code y Arquitectura:
 * - Single Responsibility Principle (SRP): Exclusivamente lectura y transmisión ambiental.
 * - FreeRTOS Watchdog Task: Autoreinicio tras desconexión crítica o cuelgue de bus.
 * - Sin retardos bloqueantes (vTaskDelay y temporizadores en base a ticks de FreeRTOS).
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

// ==============================================================================
// CONFIGURACIÓN DE RED ESP-NOW
// ==============================================================================
// Dirección MAC del Cerebro 3 (Nodo Central). Reemplazar con la MAC física del ESP32 central.
uint8_t central_node_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peer_info;

// ==============================================================================
// MAPEO DE PINES DEL CEREBRO 1 (MATRIZ AMBIENTAL)
// ==============================================================================
const int pin_internal_temp = 27;  // Sensor de Temperatura Interna del Traje (NTC / Termistor)
const int pin_humidity      = 14;  // Sensor de Humedad Relativa del Traje
const int pin_air_quality   = 12;  // Sensor de Calidad del Aire / Detección de Gases VOC
const int pin_radiation     = 13;  // Sensor de Radiación Ionizante / Detector UV

// Frecuencia de muestreo y transmisión (10 Hz = 100 ms)
const unsigned long sample_interval_ms = 100;
volatile unsigned long last_successful_read = 0;

// ==============================================================================
// TRAZABILIDAD Y LOGGING ESTRUCTURADO
// ==============================================================================
void log_info(const String& message) {
    Serial.println("[CEREBRO-1][INFO] " + message);
}

void log_error(const String& message) {
    Serial.println("[CEREBRO-1][ERROR] " + message);
}

// ==============================================================================
// TAREAS DE FREERTOS: PERRO GUARDIÁN (WATCHDOG)
// ==============================================================================
/**
 * @brief Supervisa la continuidad del flujo de sensores ambientales.
 * Si las lecturas se detienen por más de 5 segundos, fuerza el reinicio del microcontrolador.
 */
void environmental_watchdog_task(void * parameter) {
    while (true) {
        if (millis() - last_successful_read > 5000) {
            log_error("Perro Guardián: Pérdida total de sensores ambientales. Reiniciando Cerebro 1...");
            ESP.restart();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// ==============================================================================
// CALLBACKS DE TRANSMISIÓN ESP-NOW
// ==============================================================================
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        log_error("Fallo de entrega de telemetría ambiental al Cerebro 3.");
    }
}

/**
 * @brief Inicializa el módulo WiFi en modo Estación y registra el enlace ESP-NOW.
 */
void setup_espnow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        log_error("Fallo crítico inicializando protocolo ESP-NOW en Cerebro 1.");
        return;
    }

    esp_now_register_send_cb(on_data_sent);

    // Registro del nodo receptor (Cerebro 3)
    memcpy(peer_info.peer_addr, central_node_mac, 6);
    peer_info.channel = 1;     // Mismo canal WiFi que el SoftAP del Cerebro 3
    peer_info.encrypt = false; // Habilitar si se configuran llaves de cifrado WPA2

    if (esp_now_add_peer(&peer_info) != ESP_OK) {
        log_error("Fallo al registrar Cerebro 3 como peer ESP-NOW.");
        return;
    }
    log_info("Cerebro 1 emparejado exitosamente con Cerebro 3 vía ESP-NOW.");
}

// ==============================================================================
// ADQUISICIÓN Y TRANSMISIÓN DE TELEMETRÍA AMBIENTAL
// ==============================================================================
/**
 * @brief Tarea principal de muestreo ambiental en Core 1.
 */
void environmental_sampling_task(void * parameter) {
    pinMode(pin_internal_temp, INPUT);
    pinMode(pin_humidity, INPUT);
    pinMode(pin_air_quality, INPUT);
    pinMode(pin_radiation, INPUT);

    while (true) {
        unsigned long current_time = millis();

        // 1. Muestreo analógico de la matriz ambiental
        int raw_internal_temp = analogRead(pin_internal_temp);
        int raw_humidity      = analogRead(pin_humidity);
        int raw_air_quality   = analogRead(pin_air_quality);
        int raw_radiation     = analogRead(pin_radiation);

        // Actualizar temporizador de perro guardián
        last_successful_read = current_time;

        // 2. Empaquetado en formato JSON estructurado
        JsonDocument doc;
        doc["node"] = "cerebro_1";
        doc["internal_temp"] = raw_internal_temp;
        doc["humidity"] = raw_humidity;
        doc["air_quality"] = raw_air_quality;
        doc["radiation"] = raw_radiation;
        doc["timestamp"] = current_time;

        char buffer[256];
        size_t bytes_written = serializeJson(doc, buffer, sizeof(buffer));

        // 3. Envío inalámbrico no bloqueante al Cerebro 3
        esp_err_t result = esp_now_send(central_node_mac, (uint8_t *)buffer, bytes_written);
        if (result != ESP_OK) {
            log_error("Error de cola en transmisión ESP-NOW.");
        }

        vTaskDelay(sample_interval_ms / portTICK_PERIOD_MS);
    }
}

// ==============================================================================
// SETUP Y ENTRY POINT
// ==============================================================================
void setup() {
    Serial.begin(115200);
    log_info("Iniciando microcontrolador CEREBRO 1 (Matriz Ambiental)...");

    setup_espnow();
    last_successful_read = millis();

    // Crear tarea del perro guardián en Core 0
    xTaskCreatePinnedToCore(
        environmental_watchdog_task,
        "C1_Watchdog",
        2048,
        NULL,
        1,
        NULL,
        0
    );

    // Crear tarea de muestreo en Core 1
    xTaskCreatePinnedToCore(
        environmental_sampling_task,
        "C1_Sampling",
        4096,
        NULL,
        2,
        NULL,
        1
    );
}

void loop() {
    // La ejecución se delega completamente a las tareas de FreeRTOS
    vTaskDelete(NULL);
}
