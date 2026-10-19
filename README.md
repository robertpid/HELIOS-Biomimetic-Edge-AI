# H.E.L.I.O.S. — Biomimetic Edge AI Life-Support System
> **Hardware & Environment Life-support Intelligent Operating System**  
> *Inferencia Neuromórfica en el Edge (ESP32 Dual-Core) con FreeRTOS y HUD Aeroespacial Offline-First*

[![ESP32](https://img.shields.io/badge/Hardware-ESP32%20Dual--Core-red.svg?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![FreeRTOS](https://img.shields.io/badge/OS-FreeRTOS-blue.svg?style=for-the-badge&logo=freertos)](https://www.freertos.org/)
[![Edge AI](https://img.shields.io/badge/Edge%20AI-Deep%20Neural%20Network-emerald.svg?style=for-the-badge&logo=scikitlearn)](https://github.com/robertpid/HELIOS-Biomimetic-Edge-AI)
[![Python](https://img.shields.io/badge/Python-3.10%2B%20%7C%20Type--Hinted-yellow.svg?style=for-the-badge&logo=python)](https://www.python.org/)
[![C++](https://img.shields.io/badge/Firmware-C%2B%2B17%20%2F%20Arduino-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![WebSockets](https://img.shields.io/badge/Comms-Async%20WebSockets%20%40%2010Hz-orange.svg?style=for-the-badge&logo=websocket)](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API)
[![License: MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg?style=for-the-badge)](LICENSE)

---

## 🛰️ 1. Visión General del Proyecto

**H.E.L.I.O.S.** es una arquitectura ciberfísica e inteligente diseñada para misiones espaciales extravehiculares (EVA) y entornos hostiles extremos. Combina telemetría biométrica del astronauta y monitoreo ambiental del traje espacial con un **motor de inferencia neuronal embebido en el microcontrolador (Edge AI)**, eliminando por completo la dependencia de servidores en la nube, routers intermedios o conexión a Internet.

El sistema procesa en tiempo real 8 canales de telemetría física, evalúa anomalías críticas según el protocolo clínico **NEWS2** y ejecuta una **red neuronal profunda inspirada en el conectoma de la mosca (*Drosophila melanogaster*)**, emitiendo diagnósticos y sugerencias tácticas inmediatas hacia un HUD aeroespacial con latencia sub-milisegundo.

```
                      +-------------------------------------------------+
                      |              TRAJE ESPACIAL (EVA)               |
                      |                                                 |
  +---------------+   |  [SpO2, BPM, Temp, ECG, Gas, Rad, Temp, Hum]    |
  | Sensores      |-->|                         |                       |
  | Biomédicos y  |   |                         v                       |
  | Ambientales   |   |     +---------------------------------------+   |
  +---------------+   |     |           ESP32 DUAL-CORE             |   |
                      |     |  +---------------------------------+  |   |
                      |     |  | CORE 1: Edge AI & Sensing       |  |   |
                      |     |  | - ADC Sampling (10 Hz)          |  |   |
                      |     |  | - Hardware Lead-Off Check       |  |   |
                      |     |  | - Min-Max Normalization         |  |   |
                      |     |  | - DNN Matrix Inference (8x64x20)|  |   |
                      |     |  | - Tactical Decision Engine      |  |   |
                      |     |  +---------------------------------+  |   |
                      |     |                   | FreeRTOS Queue    |   |
                      |     |                   v (Non-blocking)    |   |
                      |     |  +---------------------------------+  |   |
                      |     |  | CORE 0: Async Communications   |  |   |
                      |     |  | - SoftAP (HELIOS_NETWORK)       |  |   |
                      |     |  | - Async WebServer (Port 80)     |  |   |
                      |     |  | - Async WebSockets (/ws)        |  |   |
                      |     |  +---------------------------------+  |   |
                      |     +-------------------|-------------------+   |
                      +-------------------------|-----------------------+
                                                | WiFi 802.11 b/g/n (SoftAP)
                                                v
                              +-----------------------------------+
                              |     HUD AEROESPACIAL (MÓVIL)      |
                              | - Glassmorphism UI (Offline)      |
                              | - Real-Time ECG Oscilloscope      |
                              | - Diagnóstico NEWS2 Autónomo      |
                              | - Acciones de Emergencia EVA      |
                              +-----------------------------------+
```

---

## 🧬 2. Fundamentos de Inteligencia Artificial Biomimética

La arquitectura de inferencia matemática de H.E.L.I.O.S. emula los principios de codificación dispersa (*Sparse Coding*) observados en las **células de Kenyon del cuerpo fúngico (*Mushroom Body*)** del cerebro de *Drosophila melanogaster*:

1. **Expansión a Alta Dimensión (Kenyon Cells):** El vector sensorial de entrada (8 canales) se proyecta hacia una capa intermedia densa de 64 neuronas mediante proyecciones no lineales activadas con **ReLU** ($A_1 = \max(0, Z_1)$). Esto genera una representación dispersa que desacopla anomalías compuestas.
2. **Clasificación Multietiqueta Dispersa:** La capa de salida evalúa simultáneamente **20 diagnósticos médicos y ambientales independientes** mediante funciones de activación sigmoide individuales:
   $$\sigma(z) = \frac{1}{1 + e^{-z}}$$
3. **Optimización para Microcontroladores:**
   - La red se entrena en Python con inicialización He/Xavier y optimización por descenso de gradiente.
   - La topología completa, matrices de pesos ($W_1, W_2$) y sesgos ($b_1, b_2$) se exportan a un archivo de cabecera en C++ (`pesos_nasa.h`) compuesto por constantes estáticas en memoria Flash, consumiendo **cero asignaciones dinámicas en el Heap** del microcontrolador.

---

## ⚙️ 3. Arquitectura del Firmware (ESP32 & FreeRTOS)

El firmware implementa una estricta separación de responsabilidades (**Single Responsibility Principle - SRP**) aprovechando la arquitectura Dual-Core del procesador Xtensa LX6:

### ⚡ Núcleo 0: Comunicaciones y Red Asíncrona (`core0_comms_task`)
- **Punto de Acceso Autónomo (SoftAP):** Despliega la red WiFi `HELIOS_NETWORK` (WPA2-PSK), permitiendo que cualquier dispositivo (smartphone, tablet o HUD de casco) se conecte en cualquier planeta sin requerir routers ni infraestructura terrestre.
- **Servidor Web y WebSockets Asíncronos:** Utiliza `ESPAsyncWebServer` y `AsyncWebSocket` en el puerto 80. La transmisión de telemetría a 10 Hz se realiza sin bloqueos de socket gracias a callbacks orientados a eventos.
- **Consumo de Cola IPC:** Lee paquetes serializados desde una cola de FreeRTOS (`ws_queue`) de forma no bloqueante (`pdPASS` con timeout 0), garantizando que caídas de red o clientes lentos nunca degraden la tarea de soporte vital.

### 🧠 Núcleo 1: Adquisición, Detección de Fallas y Edge AI (`core1_processing_task`)
- **Muestreo a Alta Frecuencia (10 Hz):** Lectura analógica continua de 8 canales biométricos y ambientales.
- **Flags de Calidad y Detección de Desconexión (*Lead-Off*):** Evalúa señales digitales del módulo AD8232 (`LO+` y `LO-`) y umbrales mínimos de pulso. Si los electrodos se despegan del astronauta, la red neuronal se suprime inmediatamente para evitar falsos positivos y declara un estado `FALLO HARDWARE`.
- **Inferencia Matemática Continua:** Normalización Min-Max vectorial y producto matricial con la matriz `pesos_nasa.h`.
- **Motor de Soluciones EVA:** Si la probabilidad de anomalía supera el umbral crítico (> 0.8), el sistema redacta órdenes tácticas inmediatas (ej. inyección de $O_2$ al 100%, despresurización de emergencia o aborto de misión).
- **Serialización Segura:** Los datos se formatean con `ArduinoJson` directamente en un buffer estático de 2048 bytes en el Stack, previniendo la fragmentación de memoria RAM causada por objetos `String`.

---

## 📡 4. Mapeo de Hardware y Pines (ESP32)

| Subsistema | Métrica | Sensor Sugerido | Pin ESP32 | Tipo |
| :--- | :--- | :--- | :--- | :--- |
| **Cerebro 2 (Biometría)** | Saturación $O_2$ (SpO2) | MAX30102 / Analógico | `GPIO 35` | ADC1_CH7 (Input) |
| **Cerebro 2 (Biometría)** | Frecuencia Cardíaca (BPM) | Pulso Óptico / Analógico | `GPIO 34` | ADC1_CH6 (Input) |
| **Cerebro 2 (Biometría)** | Temperatura Corporal | Termistor NTC / DS18B20 | `GPIO 32` | ADC1_CH4 (Input) |
| **Cerebro 2 (Biometría)** | Electrocardiograma (ECG) | AD8232 Heart Monitor | `GPIO 33` | ADC1_CH5 (Input) |
| **Cerebro 2 (Biometría)** | ECG Lead-Off Detect + | AD8232 LO+ | `GPIO 25` | Digital Input |
| **Cerebro 2 (Biometría)** | ECG Lead-Off Detect - | AD8232 LO- | `GPIO 26` | Digital Input |
| **Cerebro 1 (Ambiente)** | Temp. Interna del Traje | Termistor / DHT22 | `GPIO 27` | ADC2_CH7 (Input) |
| **Cerebro 1 (Ambiente)** | Humedad Relativa | Sensor Capacitivo | `GPIO 14` | ADC2_CH6 (Input) |
| **Cerebro 1 (Ambiente)** | Calidad del Aire / Gases | MQ-135 / Sensor VOC | `GPIO 12` | ADC2_CH5 (Input) |
| **Cerebro 1 (Ambiente)** | Radiación Ionizante / UV | Fotodiodo UV / Tubo GM | `GPIO 13` | ADC2_CH4 (Input) |

---

## 📊 5. Pipeline de Machine Learning (`train_biomimetic_matrix.py`)

El script de entrenamiento sintetiza y procesa datos según los estándares médicos aeroespaciales:

```
  +--------------------------------------------------------+
  |    Generación Sintética de Telemetría NASA (N=5000)     |
  |  - Distribuciones Gaussianas según líneas base humanas  |
  |  - Inyección de Anomalías Multietiqueta (20 Clases)     |
  +--------------------------------------------------------+
                             |
                             v
  +--------------------------------------------------------+
  |    Limpieza y Normalización Vectorizada (Pandas)       |
  |  - Reemplazo de Infinitos y descarte de NaNs           |
  |  - Escalado Min-Max normalizado [0.0, 1.0]             |
  +--------------------------------------------------------+
                             |
                             v
  +--------------------------------------------------------+
  |       Entrenamiento DNN [8 -> 64 -> 20]               |
  |  - Forward Pass: ReLU (Oculta) + Sigmoide (Salida)      |
  |  - Backpropagation Vectorizado con gradientes estables  |
  +--------------------------------------------------------+
                             |
                             v
  +--------------------------------------------------------+
  |       Exportación Automática a C++ (pesos_nasa.h)      |
  |  - Arrays constantes en PROGMEM / Flash                 |
  |  - Cero dependencias externas en el firmware embebido   |
  +--------------------------------------------------------+
```

### Ejecución del Pipeline
```bash
# Crear entorno virtual e instalar dependencias
python -m venv venv
source venv/bin/activate  # En Windows: venv\Scripts\activate
pip install numpy pandas

# Ejecutar el entrenamiento y compilar la matriz C++
python train_biomimetic_matrix.py
```

---

## 💻 6. HUD Aeroespacial Frontend (`frontend/`)

El HUD es una aplicación web de alta fidelidad estética desarrollada bajo principios **Vanilla (Zero Dependencies)**, diseñada para cargarse instantáneamente desde la memoria SPIFFS del ESP32 o desde cualquier dispositivo móvil:

- **Estética Cyberpunk / Aerospace Glassmorphism:** Fondo degradado espacial, paneles de vidrio con `backdrop-filter: blur(12px)`, tipografías técnicas (`Orbitron` e `Inter`) y bordes cian reactivos.
- **Osciloscopio ECG en Tiempo Real:** Renderizado continuo a 60 FPS sobre HTML5 `<canvas>` con interpolación suave y buffer circular de 150 puntos.
- **Motor Clínico NEWS2 Integrado:** Clasifica instantáneamente estados de hipoxia ($SpO_2 < 92\%$), taquicardia severa ($BPM > 130$), hipertermia ($T > 38.5^\circ\text{C}$) o sobreexposición a radiación UV, disparando transiciones visuales de alarma (`alert-state`).
- **Resiliencia y Auto-Reconexión:** Socket inteligente con latencímetro en vivo y reconexión automática tras caídas de señal.

```bash
# Para previsualizar el HUD localmente:
cd frontend
python -m http.server 8080
# Abrir en el navegador: http://localhost:8080
```

---

## 📂 7. Estructura del Repositorio

```text
H.E.L.I.O/
├── .gitignore                    # Reglas de exclusión de compilación y temporales
├── README.md                     # Documentación técnica integral
├── central_node.cpp              # Firmware principal ESP32 (Dual-Core + Edge AI + WebSockets)
├── peripheral_node.cpp           # Firmware secundario para nodos distribuidos (ESP-NOW + Watchdog)
├── pesos_nasa.h                  # Cabecera C++ con matrices neuronales preentrenadas
├── train_biomimetic_matrix.py    # Pipeline de Machine Learning (NumPy, Pandas, Clean Code)
└── frontend/                     # Aplicación HUD Aeroespacial Offline-First
    ├── index.html                # Estructura semántica y paneles de telemetría
    ├── style.css                 # Diseño Glassmorphism aeroespacial y animaciones
    └── app.js                    # Cliente WebSocket, osciloscopio ECG y diagnósticos
```

---

## 🛠️ 8. Compilación y Despliegue del Firmware

### Requisitos Previos
1. **IDE:** [PlatformIO](https://platformio.org/) (recomendado en VSCode) o Arduino IDE con soporte para placa ESP32.
2. **Librerías Necesarias:**
   - `ESPAsyncWebServer` (por me-no-dev / mathieucarbou)
   - `AsyncTCP`
   - `ArduinoJson` (v6 o v7)
   - `WiFi` (integrada en ESP32 core)

### Configuración de `platformio.ini` (Ejemplo)
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson @ ^7.0.0
    https://github.com/mathieucarbou/ESPAsyncWebServer.git
    https://github.com/mathieucarbou/AsyncTCP.git
```

### Pasos de Puesta en Marcha
1. Flashea `central_node.cpp` en tu ESP32 asegurando que `pesos_nasa.h` esté en el mismo directorio de compilación.
2. Abre el monitor serie a **115200 baudios**.
3. Desde tu smartphone, laptop o tablet, conéctate a la red WiFi:
   - **SSID:** `HELIOS_NETWORK`
   - **Contraseña:** `HELIOS_PASSWORD`
4. Abre el archivo `frontend/index.html` en el navegador del dispositivo conectado. El HUD se sincronizará automáticamente con el WebSocket del traje (`ws://192.168.4.1/ws`).

---

## 🛡️ 9. Principios de Ingeniería de Software (Clean Code)

- **Arquitectura Modular (SRP):** Tareas asíncronas separadas estrictamente por núcleo de procesamiento (Core 1 para Hardware y Red Neuronal; Core 0 para Comunicaciones WiFi).
- **Tipado Estático Riguroso:** El código Python implementa Type Hinting completo (`Tuple`, `pd.DataFrame`, `np.ndarray`, etc.).
- **Trazabilidad Profesional:** Prohibido el uso de `print()` no estructurado; se utiliza el módulo `logging` de Python (niveles INFO y ERROR con captura de excepciones específicas) y macros dedicadas `log_info()` y `log_error()` en C++.
- **Operaciones Vectorizadas en Pandas:** Transformaciones numéricas sin bucles iterativos (`iterrows()` / `itertuples()`).
- **Seguridad en Memoria:** Ausencia de bloqueos por `delay()` mediante el uso de timers y ticks de FreeRTOS (`vTaskDelay`). Eliminación de fragmentación en Heap serializando JSONs directamente en buffers de Stack.

---

## 📜 10. Licencia

Este proyecto está distribuido bajo la licencia **MIT**. Consulta el archivo `LICENSE` para más información.

---

**Desarrollado con dedicación a la exploración espacial y la computación biomimética al límite.**  
Autor: [@robertpid](https://github.com/robertpid)
