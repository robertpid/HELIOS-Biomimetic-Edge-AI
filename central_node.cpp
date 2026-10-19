/**
 * @file central_node.cpp
 * @brief Proyecto H.E.L.I.O.S. - Arquitectura Pivote (Protocolo EVA / NEWS2)
 * Servidor WebSocket Asíncrono en WIFI_AP. 
 * Núcleo 0: Comunicaciones | Núcleo 1: Procesamiento de Red Neuronal Profunda (DNN)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <math.h>
#include "pesos_nasa.h"

const char* ap_ssid = "HELIOS_NETWORK";
const char* ap_password = "HELIOS_PASSWORD";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Pines Cerebro 2 (Biometría del Astronauta - Standalone Fallback)
const int pin_spo2 = 35;
const int pin_bpm = 34;
const int pin_body_temp = 32;
const int pin_ecg = 33;
const int pin_ecg_lo_plus = 25;
const int pin_ecg_lo_minus = 26;

// Pines Cerebro 1 (Matriz Ambiental - Standalone Fallback)
const int pin_internal_temp = 27;
const int pin_humidity = 14;
const int pin_air_quality = 12;
const int pin_radiation = 13;

// Telemetría compartida y control distribuido
unsigned long last_telemetry_time = 0;
bool sensor_valid = true;
volatile bool has_espnow_c1 = false;
volatile bool has_espnow_c2 = false;

// Estado
#define MAX_DIAGNOSTICS 50 // Escalonado para soportar EVAs de alta complejidad
float state_vector[L1_ROWS] = {0};
float current_diagnostics[MAX_DIAGNOSTICS] = {0};
char current_solution[256] = "Astronauta Estable. Ninguna anomalia detectada.";

void log_info(const String& message) { Serial.println("[INFO] " + message); }
void log_error(const String& message) { Serial.println("[ERROR] " + message); }

void on_ws_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) log_info("Conexión entrante al HUD (Smartphone).");
    else if (type == WS_EVT_DISCONNECT) log_info("HUD desconectado.");
}

QueueHandle_t ws_queue;

/**
 * @brief Callback de recepción ESP-NOW para Nodos Periféricos (Cerebro 1 y Cerebro 2)
 */
void on_espnow_recv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, incoming_data, len);
    if (!error) {
        const char* node = doc["node"];
        if (node && strcmp(node, "cerebro_1") == 0) {
            state_vector[4] = doc["internal_temp"] | 0;
            state_vector[5] = doc["humidity"] | 0;
            state_vector[6] = doc["air_quality"] | 0;
            state_vector[7] = doc["radiation"] | 0;
            has_espnow_c1 = true;
        } else if (node && strcmp(node, "cerebro_2") == 0) {
            state_vector[0] = doc["spo2"] | 0;
            state_vector[1] = doc["bpm"] | 0;
            state_vector[2] = doc["body_temp"] | 0;
            state_vector[3] = doc["ecg"] | 0;
            sensor_valid = doc["sensor_valid"] | true;
            has_espnow_c2 = true;
        }
    }
}

void core0_comms_task(void * parameter) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid, ap_password, 1); // Canal 1 para convivir con ESP-NOW

    // Inicializar receptor ESP-NOW para Cerebro 1 y Cerebro 2
    if (esp_now_init() == ESP_OK) {
        esp_now_register_recv_cb(on_espnow_recv);
        log_info("ESP-NOW receptor activado. Listo para enlazar Cerebro 1 y Cerebro 2.");
    } else {
        log_error("Fallo inicializando ESP-NOW receptor en Cerebro 3.");
    }

    ws.onEvent(on_ws_event);
    server.addHandler(&ws);
    server.begin();
    
    while (true) {
        char buffer[2048]; // Aumentado para soportar telemetría pesada
        if (xQueueReceive(ws_queue, &buffer, 0) == pdPASS) {
            ws.textAll(buffer);
        }
        ws.cleanupClients();
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}

/**
 * @brief Inferencia Biomimética Multicapa (Simulación Cerebro Mosca)
 */
void process_edge_ai() {
    if (!sensor_valid) {
        log_error("Supresión de Red Neuronal Profunda por POOR/INVALID Quality Flag.");
        for(int i=0; i<L2_COLS; i++) current_diagnostics[i] = -1.0;
        snprintf(current_solution, sizeof(current_solution), "ALERTA EVA: FALLO SENSOR - REVISAR CONEXIONES EN TRAJE");
        return;
    }
    
    // Sub-nivel 1: Capa Oculta Expandida (Kenyon Cells - Sparse Coding)
    float hidden_layer[L1_COLS] = {0};
    for (int i = 0; i < L1_COLS; ++i) {
        hidden_layer[i] = b1[i]; // Bias
        for (int j = 0; j < L1_ROWS; ++j) {
            hidden_layer[i] += state_vector[j] * W1[j][i];
        }
        // Activación ReLU
        if (hidden_layer[i] < 0.0) {
            hidden_layer[i] = 0.0;
        }
    }
    
    // Sub-nivel 2: Capa de Salida Multidiagnóstico
    for (int k = 0; k < L2_COLS; ++k) {
        float output_layer = b2[k];
        for (int i = 0; i < L2_ROWS; ++i) {
            output_layer += hidden_layer[i] * W2[i][k];
        }
        // Activación Sigmoide por clase
        current_diagnostics[k] = 1.0 / (1.0 + exp(-output_layer));
    }

    // Instintos de Supervivencia Biomiméticos
    bool anomaly_found = false;
    // Evaluamos los diagnósticos base conocidos (0, 1, 2)
    if (L2_COLS > 0 && current_diagnostics[0] > 0.8) {
        snprintf(current_solution, sizeof(current_solution), "HIPOXIA CRITICA. Accion: Inyectar O2 100%% en traje EVA.");
        anomaly_found = true;
    } else if (L2_COLS > 1 && current_diagnostics[1] > 0.8) {
        snprintf(current_solution, sizeof(current_solution), "ARRESTO CARDIACO INMINENTE. Accion: Preparar desfibrilador y abortar EVA.");
        anomaly_found = true;
    } else if (L2_COLS > 2 && current_diagnostics[2] > 0.8) {
        snprintf(current_solution, sizeof(current_solution), "ALERTA AMBIENTAL. Accion: Evacuacion a modulo presurizado.");
        anomaly_found = true;
    } else {
        // Evaluamos diagnósticos dinámicos (del 3 hasta L2_COLS) si los añades a la matriz
        for (int i = 3; i < L2_COLS; i++) {
            if (current_diagnostics[i] > 0.8) {
                snprintf(current_solution, sizeof(current_solution), "ANOMALIA DETECTADA (Codigo %d). Accion: Monitorizar signos vitales.", i);
                anomaly_found = true;
                break;
            }
        }
    }
    
    if (!anomaly_found) {
        snprintf(current_solution, sizeof(current_solution), "Astronauta Estable. Mision EVA procediendo normal.");
    }
}

void core1_processing_task(void * parameter) {
    pinMode(pin_ecg_lo_plus, INPUT);
    pinMode(pin_ecg_lo_minus, INPUT);
    
    while (true) {
        unsigned long current_time = millis();
        
        // 1. Adquisición: Si se reciben paquetes inalámbricos de Cerebro 1 y Cerebro 2,
        // se respeta su telemetría. De lo contrario, se leen los pines locales (Fallback Standalone).
        if (!has_espnow_c2) {
            state_vector[0] = analogRead(pin_spo2);
            state_vector[1] = analogRead(pin_bpm);
            state_vector[2] = analogRead(pin_body_temp);
            state_vector[3] = analogRead(pin_ecg);
            
            // Validación local de electrodos y señal
            sensor_valid = true;
            if (digitalRead(pin_ecg_lo_plus) == 1 || digitalRead(pin_ecg_lo_minus) == 1 || state_vector[0] < 100) {
                sensor_valid = false;
            }
        }

        if (!has_espnow_c1) {
            state_vector[4] = analogRead(pin_internal_temp);
            state_vector[5] = analogRead(pin_humidity);
            state_vector[6] = analogRead(pin_air_quality);
            state_vector[7] = analogRead(pin_radiation);
        }
        
        // 3. Tolerancia de Latencia
        if (current_time - last_telemetry_time > 3000 && last_telemetry_time != 0) {
            log_error("Tolerancia de Latencia: Pérdida Crítica de Telemetría (Superado 3.0s)");
        } else {
            last_telemetry_time = current_time; 
        }
        
        // Normalización Min-Max para compatibilidad con la Red Neuronal
        for (int i = 0; i < L1_ROWS; i++) {
            state_vector[i] = (state_vector[i] - MIN_VALS[i]) / (MAX_VALS[i] - MIN_VALS[i] + 1e-8);
        }

        // 4. Procesamiento de Red Neuronal Profunda
        process_edge_ai();
        
        // 5. Empaquetado JSON
        JsonDocument doc; 
        doc["spo2"] = state_vector[0];
        doc["bpm"] = state_vector[1];
        doc["body_temp"] = state_vector[2];
        doc["ecg"] = state_vector[3];
        doc["internal_temp"] = state_vector[4];
        doc["humidity"] = state_vector[5];
        doc["air_quality"] = state_vector[6];
        doc["radiation"] = state_vector[7];
        doc["sensor_valid"] = sensor_valid;
        // Empaquetado dinámico de N diagnósticos para telemetría a Tierra / HUD
        JsonArray diags = doc.createNestedArray("diagnostics_array");
        for (int i = 0; i < L2_COLS; i++) {
            diags.add(current_diagnostics[i]);
        }
        doc["solution"] = current_solution;
        doc["timestamp"] = current_time;
        
        // 6. Despacho (Seguro entre hilos vía Queue)
        char buffer[2048];
        // Serializamos directamente al buffer (evita fragmentación del Heap por la clase String)
        serializeJson(doc, buffer);
        
        // Timeout 0 (Non-blocking IPC). Evita el bloqueo perpetuo del Core 1 si la red falla.
        if (xQueueSend(ws_queue, &buffer, 0) != pdPASS) {
            log_error("Cola WiFi llena. Paquete JSON descartado (Soporte Vital ininterrumpido).");
        }
        
        vTaskDelay(100 / portTICK_PERIOD_MS); // 10Hz
    }
}

void setup() {
    Serial.begin(115200);
    // Cola expandida a 2048 bytes x 10 slots = 20KB (Soporta JSONs masivos sin romper la RAM)
    ws_queue = xQueueCreate(10, 2048);
    xTaskCreatePinnedToCore(core0_comms_task, "Core0_Comms", 8192, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(core1_processing_task, "Core1_EdgeAI", 8192, NULL, 1, NULL, 1);
}

void loop() {
    vTaskDelete(NULL);
}
