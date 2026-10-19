/**
 * @file peripheral_node.cpp
 * @brief Script para el Cerebro 1/2 (Nodo Periférico). 
 * Lee sensores analógicos simulados (oxígeno, ritmo cardíaco),
 * empaqueta en JSON y los envía mediante ESP-NOW al Cerebro 3.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>

// MAC address del Cerebro 3 (Reemplazar por la MAC real del Nodo Central)
uint8_t central_node_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_peer_info_t peer_info;

// Pines de sensores simulados (pueden ser pines I2C si se usa MAX30102 real)
const int heart_rate_pin = 34;
const int oxygen_pin = 35;

// Variables globales para manejo de tiempo sin delay()
volatile unsigned long previous_millis = 0;
const unsigned long read_interval_ms = 1000;
volatile unsigned long last_successful_read = 0;

/**
 * @brief Funciones de logging en lugar de usar Serial.print directamente.
 * Sigue la regla de trazabilidad.
 */
void log_info(const String& message) {
    Serial.println("[INFO] " + message);
}

void log_error(const String& message) {
    Serial.println("[ERROR] " + message);
}

/**
 * @brief Tarea de FreeRTOS que actúa como perro guardián.
 * Reinicia el sistema si los sensores fallan o se desconectan.
 */
void watchdog_task(void * parameters) {
    while (true) {
        if (millis() - last_successful_read > 5000) {
            log_error("Perro guardián: Sensores desconectados o bloqueados. Reiniciando nodo...");
            ESP.restart(); // Reinicio del sistema en caso de falla crítica
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Callback cuando se envían datos por ESP-NOW.
 */
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status != ESP_NOW_SEND_SUCCESS) {
        log_error("Fallo en la entrega de paquete ESP-NOW al Cerebro 3");
    }
}

/**
 * @brief Inicializa el protocolo ESP-NOW.
 */
void setup_espnow() {
    WiFi.mode(WIFI_STA); // ESP-NOW requiere modo estación
    
    if (esp_now_init() != ESP_OK) {
        log_error("Error inicializando ESP-NOW");
        return;
    }
    
    esp_now_register_send_cb(on_data_sent);
    
    // Registrar el peer (Nodo Central)
    memcpy(peer_info.peer_addr, central_node_mac, 6);
    peer_info.channel = 0;  
    peer_info.encrypt = false; // Ajustar a true si hay llaves configuradas
    
    if (esp_now_add_peer(&peer_info) != ESP_OK){
        log_error("Fallo al agregar el nodo central (Cerebro 3)");
        return;
    }
    log_info("ESP-NOW configurado exitosamente. Emparejado con Cerebro 3.");
}

/**
 * @brief Simula la lectura del sensor de ritmo cardíaco.
 */
int read_heart_rate() {
    return analogRead(heart_rate_pin);
}

/**
 * @brief Simula la lectura del sensor de oxígeno.
 */
int read_oxygen() {
    return analogRead(oxygen_pin);
}

/**
 * @brief Procesa los datos de los sensores, arma el JSON y lo transmite.
 */
void process_sensor_data() {
    unsigned long current_millis = millis();
    
    // Evita el uso de delay() mediante temporizador millis()
    if (current_millis - previous_millis >= read_interval_ms) {
        previous_millis = current_millis;
        
        int hr_val = read_heart_rate();
        int o2_val = read_oxygen();
        
        // Si la lectura es exitosa, refrescamos al perro guardián
        if (hr_val > 0 || o2_val > 0) {
            last_successful_read = current_millis;
        }

        // Empaquetado JSON ligero
        JsonDocument doc; 
        doc["heart_rate"] = hr_val;
        doc["oxygen_level"] = o2_val;
        doc["timestamp"] = current_millis;
        
        String json_string;
        serializeJson(doc, json_string);
        
        // Enviar por ESP-NOW (Max 250 bytes permitidos por paquete)
        esp_err_t result = esp_now_send(central_node_mac, (uint8_t *)json_string.c_str(), json_string.length());
        
        if (result == ESP_OK) {
            log_info("Telemetría enviada: " + json_string);
        } else {
            log_error("Error de transmisión ESP-NOW");
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    setup_espnow();
    last_successful_read = millis(); // Inicializa temporizador del perro guardián
    
    // Inicia perro guardián en segundo plano
    xTaskCreate(
        watchdog_task,
        "Watchdog_Task",
        2048,
        NULL,
        1,
        NULL
    );
}

void loop() {
    process_sensor_data();
}
