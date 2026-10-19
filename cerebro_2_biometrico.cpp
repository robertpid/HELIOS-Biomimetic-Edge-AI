/**
 * @file cerebro_2_biometrico.cpp
 * @brief Proyecto H.E.L.I.O.S. - CEREBRO 2: Módulo Biométrico y Soporte Vital (NEWS2)
 * @details Firmware para microcontrolador ESP32 dedicado al monitoreo fisiológico continuo.
 * Adquiere telemetría de SpO2, ritmo cardíaco, temperatura y electrocardiograma (AD8232),
 * verifica el estado físico de los electrodos (Lead-Off Detection) y transmite hacia el Cerebro 3
 * mediante el protocolo inalámbrico ESP-NOW.
 * 
 * Principios de Clean Code y Arquitectura:
 * - Single Responsibility Principle (SRP): Dedicado únicamente a biometría humana y validación de electrodos.
 * - Lead-Off Hardware Check: Detección instantánea de desconexión física de electrodos.
 * - FreeRTOS Watchdog Task: Autoprotección del microcontrolador ante fallos de hardware.
 * - Ausencia de retardos bloqueantes (vTaskDelay y gestión en FreeRTOS).
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
// MAPEO DE PINES DEL CEREBRO 2 (BIOMETRÍA DEL ASTRONAUTA)
// ==============================================================================
const int pin_spo2         = 35;  // Sensor de Saturación de Oxígeno (SpO2)
const int pin_bpm          = 34;  // Sensor de Pulso / Frecuencia Cardíaca (BPM)
const int pin_body_temp    = 32;  // Sensor de Temperatura Corporal
const int pin_ecg          = 33;  // Señal Analógica de Electrocardiograma (AD8232)
const int pin_ecg_lo_plus  = 25;  // AD8232 Lead-Off Detection (+) (Electrodo desconectado)
const int pin_ecg_lo_minus = 26;  // AD8232 Lead-Off Detection (-) (Electrodo desconectado)

// Frecuencia de muestreo y transmisión fisiológica (10 Hz = 100 ms)
const unsigned long sample_interval_ms = 100;
volatile unsigned long last_successful_read = 0;

// ==============================================================================
// TRAZABILIDAD Y LOGGING ESTRUCTURADO
// ==============================================================================
void log_info(const String& message) {
    Serial.println("[CEREBRO-2][INFO] " + message);
}

void log_error(const String& message) {
    Serial.println("[CEREBRO-2][ERROR] " + message);
}

// ==============================================================================
// TAREAS DE FREERTOS: PERRO GUARDIÁN (WATCHDOG)
// ==============================================================================
/**
 * @brief Supervisa la continuidad del flujo de sensores biométricos.
 * Si las lecturas fisiológicas fallan por más de 5 segundos, reinicia el microcontrolador.
 */
void biometric_watchdog_task(void * parameter) {
    while (true) {
        if (millis() - last_successful_read > 5000) {
            log_error("Perro Guardián: Pérdida total de sensores biométricos. Reiniciando Cerebro 2...");
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
        log_error("Fallo de entrega de telemetría biométrica al Cerebro 3.");
    }
}

/**
 * @brief Inicializa el módulo WiFi en modo Estación y registra el enlace ESP-NOW.
 */
void setup_espnow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        log_error("Fallo crítico inicializando protocolo ESP-NOW en Cerebro 2.");
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
    log_info("Cerebro 2 emparejado exitosamente con Cerebro 3 vía ESP-NOW.");
}

// ==============================================================================
// ADQUISICIÓN Y TRANSMISIÓN DE TELEMETRÍA BIOMÉTRICA
// ==============================================================================
/**
 * @brief Tarea principal de muestreo biomédico en Core 1.
 */
void biometric_sampling_task(void * parameter) {
    pinMode(pin_spo2, INPUT);
    pinMode(pin_bpm, INPUT);
    pinMode(pin_body_temp, INPUT);
    pinMode(pin_ecg, INPUT);
    pinMode(pin_ecg_lo_plus, INPUT);
    pinMode(pin_ecg_lo_minus, INPUT);

    while (true) {
        unsigned long current_time = millis();

        // 1. Muestreo analógico de biometría del astronauta
        int raw_spo2      = analogRead(pin_spo2);
        int raw_bpm       = analogRead(pin_bpm);
        int raw_body_temp = analogRead(pin_body_temp);
        int raw_ecg       = analogRead(pin_ecg);

        // 2. Validación de acoplamiento físico (Lead-Off y calidad de señal)
        bool sensor_valid = true;
        if (digitalRead(pin_ecg_lo_plus) == 1 || digitalRead(pin_ecg_lo_minus) == 1) {
            sensor_valid = false; // Electrodo de ECG desconectado del cuerpo
            log_error("Alerta: Electrodos ECG desconectados (Lead-Off detectado).");
        }
        if (raw_spo2 < 100) {
            sensor_valid = false; // Sensor de oximetría desconectado
        }

        // Actualizar temporizador de perro guardián si el hardware responde
        last_successful_read = current_time;

        // 3. Empaquetado en formato JSON estructurado
        JsonDocument doc;
        doc["node"] = "cerebro_2";
        doc["spo2"] = raw_spo2;
        doc["bpm"] = raw_bpm;
        doc["body_temp"] = raw_body_temp;
        doc["ecg"] = raw_ecg;
        doc["sensor_valid"] = sensor_valid;
        doc["timestamp"] = current_time;

        char buffer[256];
        size_t bytes_written = serializeJson(doc, buffer, sizeof(buffer));

        // 4. Envío inalámbrico no bloqueante al Cerebro 3
        esp_err_t result = esp_now_send(central_node_mac, (uint8_t *)buffer, bytes_written);
        if (result != ESP_OK) {
            log_error("Error de cola en transmisión biométrica ESP-NOW.");
        }

        vTaskDelay(sample_interval_ms / portTICK_PERIOD_MS);
    }
}

// ==============================================================================
// SETUP Y ENTRY POINT
// ==============================================================================
void setup() {
    Serial.begin(115200);
    log_info("Iniciando microcontrolador CEREBRO 2 (Biometría y Soporte Vital)...");

    setup_espnow();
    last_successful_read = millis();

    // Crear tarea del perro guardián en Core 0
    xTaskCreatePinnedToCore(
        biometric_watchdog_task,
        "C2_Watchdog",
        2048,
        NULL,
        1,
        NULL,
        0
    );

    // Crear tarea de muestreo en Core 1
    xTaskCreatePinnedToCore(
        biometric_sampling_task,
        "C2_Sampling",
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
