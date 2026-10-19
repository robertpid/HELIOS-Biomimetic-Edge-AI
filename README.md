# H.E.L.I.O.S. — Biomimetic Edge AI Life-Support System
> **Hardware & Environment Life-support Intelligent Operating System**  
> *Inferencia Neuromórfica Distribuida en el Edge (ESP32 Multi-Node + FreeRTOS) y HUD Aeroespacial Offline-First*

[![ESP32](https://img.shields.io/badge/Hardware-ESP32%20Multi--Node-red.svg?style=for-the-badge&logo=espressif)](https://www.espressif.com/)
[![FreeRTOS](https://img.shields.io/badge/OS-FreeRTOS-blue.svg?style=for-the-badge&logo=freertos)](https://www.freertos.org/)
[![Edge AI](https://img.shields.io/badge/Edge%20AI-Deep%20Neural%20Network-emerald.svg?style=for-the-badge&logo=scikitlearn)](https://github.com/robertpid/HELIOS-Biomimetic-Edge-AI)
[![Python](https://img.shields.io/badge/Python-3.10%2B%20%7C%20Type--Hinted-yellow.svg?style=for-the-badge&logo=python)](https://www.python.org/)
[![C++](https://img.shields.io/badge/Firmware-C%2B%2B17%20%2F%20Arduino-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![ESP-NOW](https://img.shields.io/badge/Inter--Node-ESP--NOW%20%40%2010Hz-purple.svg?style=for-the-badge&logo=wi-fi)](https://www.espressif.com/en/solutions/low-power-solutions/esp-now)
[![WebSockets](https://img.shields.io/badge/Comms-Async%20WebSockets%20%40%20Port%2080-orange.svg?style=for-the-badge&logo=websocket)](https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API)
[![License: MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg?style=for-the-badge)](LICENSE)

---

## 🛰️ 1. Visión General del Proyecto

**H.E.L.I.O.S.** es una arquitectura ciberfísica e inteligente diseñada para misiones espaciales extravehiculares (EVA) y entornos hostiles extremos. El sistema implementa una **topología distribuida de 3 microcontroladores independientes (los 3 Cerebros)** comunicados a través del protocolo de baja latencia **ESP-NOW**, integrados por un nodo central que ejecuta una **red neuronal profunda en el microcontrolador (Edge AI)** inspirada en el conectoma de la mosca (*Drosophila melanogaster*).

El sistema opera **100% desconectado de Internet** (offline-first), eliminando cuellos de botella y riesgos de desconexión en el espacio exterior.

```
                  +-------------------------------------------------+
                  |       ARQUITECTURA DE LOS 3 CEREBROS (EVA)      |
                  +-------------------------------------------------+

  [CEREBRO 1: MICROCONTROLADOR AMBIENTAL]
  (cerebro_1_ambiental.cpp - ESP32 Dedicado)
  - Temp. Interna Traje (GPIO 27)
  - Humedad Relativa (GPIO 14)
  - Calidad del Aire / Gases VOC (GPIO 12)
  - Radiación Ionizante / UV (GPIO 13)
  - FreeRTOS Watchdog Task
         |
         | ESP-NOW (Canal 1 / 10 Hz)
         v
  +-----------------------------------------------------------------+
  |        CEREBRO 3: NODO CENTRAL Y MOTOR EDGE AI (ESP32)          |
  |                    (central_node.cpp)                           |
  |                                                                 |
  |  +-----------------------------------------------------------+  |
  |  | CORE 0: Comunicaciones y Enlace Inalámbrico               |  |
  |  | - Receptor ESP-NOW (Fusión C1 + C2)                       |  |
  |  | - SoftAP Autónomo (HELIOS_NETWORK)                        |  |
  |  | - Servidor WebSockets Asíncrono (/ws @ Port 80)           |  |
  |  +-----------------------------------------------------------+  |
  |                               | FreeRTOS Queue (No bloqueante)   |
  |                               v                                 |
  |  +-----------------------------------------------------------+  |
  |  | CORE 1: Inferencia Biomimética y Supervisión              |  |
  |  | - Normalización Vectorial Min-Max                         |  |
  |  | - Inferencia Red Neuronal Profunda (8 -> 64 -> 20)        |  |
  |  | - Evaluación Clínica NEWS2 y Directivas EVA               |  |
  |  +-----------------------------------------------------------+  |
  +-----------------------------------------------------------------+
         ^
         | ESP-NOW (Canal 1 / 10 Hz)
         |
  [CEREBRO 2: MICROCONTROLADOR BIOMÉTRICO]
  (cerebro_2_biometrico.cpp - ESP32 Dedicado)
  - Saturación SpO2 (GPIO 35)
  - Frecuencia Cardíaca BPM (GPIO 34)
  - Temperatura Corporal (GPIO 32)
  - Electrocardiograma ECG AD8232 (GPIO 33)
  - Detección Lead-Off LO+/LO- (GPIO 25, 26)
  - FreeRTOS Watchdog Task
         |
         +----------------------------+
                                      | WiFi SoftAP (802.11 b/g/n)
                                      v
                    +-----------------------------------+
                    |     HUD AEROESPACIAL (MÓVIL)      |
                    |        (frontend/)                |
                    | - Glassmorphism UI (Offline)      |
                    | - Real-Time ECG Oscilloscope      |
                    | - Diagnóstico NEWS2 Autónomo      |
                    | - Acciones Tácticas Inmediatas    |
                    +-----------------------------------+
```

---

## 🧩 2. Distribución de los 3 Cerebros (Microcontroladores)

El sistema se divide físicamente en tres unidades de cómputo especializadas, cumpliendo con el principio de responsabilidad única (**SRP**):

### 🌡️ Cerebro 1: Módulo Ambiental del Traje (`cerebro_1_ambiental.cpp`)
- **Hardware:** Microcontrolador ESP32 dedicado montado en la estructura del traje.
- **Pines y Sensores:**
  - `GPIO 27`: Temperatura interna del traje espacial.
  - `GPIO 14`: Humedad relativa interna.
  - `GPIO 12`: Calidad del aire y detección de gases tóxicos / VOC (MQ-135).
  - `GPIO 13`: Radiación ionizante / sensor UV.
- **Protocolo:** Transmite paquetes estructurados vía **ESP-NOW** a 10 Hz hacia el Cerebro 3.
- **Protección:** Tarea de FreeRTOS `environmental_watchdog_task` que reinicia el chip en 5 segundos si los sensores sufren fallo de bus.

### ❤️ Cerebro 2: Módulo Biométrico y Soporte Vital (`cerebro_2_biometrico.cpp`)
- **Hardware:** Microcontrolador ESP32 dedicado adherido a la vestimenta biomédica del astronauta.
- **Pines y Sensores:**
  - `GPIO 35`: Saturación de oxígeno en sangre ($SpO_2$).
  - `GPIO 34`: Pulso y frecuencia cardíaca (BPM).
  - `GPIO 32`: Temperatura corporal periférica/central.
  - `GPIO 33`: Señal analógica de electrocardiograma continuo (AD8232).
  - `GPIO 25` y `GPIO 26`: Pines digitales `LO+` / `LO-` de detección de electrodos desprendidos (*Lead-Off Detection*).
- **Protocolo:** Transmite paquetes biométricos validados vía **ESP-NOW** a 10 Hz hacia el Cerebro 3.
- **Protección:** Si los electrodos se despegan del cuerpo, conmuta el flag `sensor_valid = false` para alertar al sistema y suprimir diagnósticos erróneos de la IA.

### 🧠 Cerebro 3: Nodo Central, Inferencia y Servidor (`central_node.cpp`)
- **Hardware:** Microcontrolador ESP32 Dual-Core con FreeRTOS (Cerebro Maestro).
- **Núcleo 0 (Comunicaciones):**
  - Receptor asíncrono **ESP-NOW** que fusiona los paquetes del Cerebro 1 y Cerebro 2.
  - Despliega el punto de acceso WiFi `HELIOS_NETWORK` (WPA2-PSK).
  - Servidor `ESPAsyncWebServer` y `AsyncWebSocket` en el puerto 80 hacia el HUD móvil.
- **Núcleo 1 (Edge AI y Supervisión):**
  - Fusión de las 8 variables físicas normalizadas.
  - Inferencia de la Red Neuronal Profunda con pesos de `pesos_nasa.h`.
  - Evaluación clínica según el protocolo **NEWS2** y despacho de sugerencias tácticas.
  - *Modo Fallback:* Si no detecta nodos inalámbricos, puede muestrear directamente los pines locales para pruebas en una sola placa.

---

## 🧬 3. Fundamentos de Inteligencia Artificial Biomimética

La arquitectura matemática de H.E.L.I.O.S. emula los principios de codificación dispersa (*Sparse Coding*) observados en las **células de Kenyon del cuerpo fúngico (*Mushroom Body*)** del cerebro de *Drosophila melanogaster*:

1. **Expansión a Alta Dimensión (Kenyon Cells):** El vector sensorial de entrada (8 canales) se proyecta hacia una capa intermedia densa de 64 neuronas mediante proyecciones no lineales activadas con **ReLU** ($A_1 = \max(0, Z_1)$). Esto genera una representación dispersa que desacopla anomalías compuestas.
2. **Clasificación Multietiqueta Dispersa:** La capa de salida evalúa simultáneamente **20 diagnósticos médicos y ambientales independientes** mediante funciones de activación sigmoide individuales:
   $$\sigma(z) = \frac{1}{1 + e^{-z}}$$
3. **Optimización para Microcontroladores:**
   - La red se entrena en Python con inicialización He/Xavier y optimización por descenso de gradiente.
   - La topología completa, matrices de pesos ($W_1, W_2$) y sesgos ($b_1, b_2$) se exportan a un archivo de cabecera en C++ (`pesos_nasa.h`) compuesto por constantes estáticas en memoria Flash, consumiendo **cero asignaciones dinámicas en el Heap** del microcontrolador.

---

## 📊 4. Pipeline de Machine Learning (`train_biomimetic_matrix.py`)

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

## 💻 5. HUD Aeroespacial Frontend (`frontend/`)

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

## 📂 6. Estructura del Repositorio

```text
HELIOS-Biomimetic-Edge-AI/
├── .gitignore                    # Reglas de exclusión de compilación y temporales
├── LICENSE                       # Licencia MIT del proyecto
├── README.md                     # Documentación técnica integral del sistema
├── cerebro_1_ambiental.cpp       # [Cerebro 1] Firmware ESP32 para Matriz Ambiental (ESP-NOW)
├── cerebro_2_biometrico.cpp       # [Cerebro 2] Firmware ESP32 para Biometría y Soporte Vital (ESP-NOW)
├── central_node.cpp              # [Cerebro 3] Nodo Central ESP32 (Receptor + Edge AI + WebSockets)
├── pesos_nasa.h                  # Cabecera C++ con matrices neuronales preentrenadas
├── train_biomimetic_matrix.py    # Pipeline de Machine Learning (NumPy, Pandas, Clean Code)
└── frontend/                     # Aplicación HUD Aeroespacial Offline-First
    ├── index.html                # Estructura semántica y paneles de telemetría
    ├── style.css                 # Diseño Glassmorphism aeroespacial y animaciones
    └── app.js                    # Cliente WebSocket, osciloscopio ECG y diagnósticos
```

---

## 🛠️ 7. Compilación y Despliegue del Firmware

### Requisitos Previos
1. **IDE:** [PlatformIO](https://platformio.org/) (recomendado en VSCode) o Arduino IDE con soporte para placa ESP32.
2. **Librerías Necesarias:**
   - `ESPAsyncWebServer` (en Cerebro 3)
   - `AsyncTCP` (en Cerebro 3)
   - `ArduinoJson` v7 o v6 (en Cerebro 1, 2 y 3)
   - `WiFi` y `esp_now` (integradas en el core de ESP32)

### Despliegue en los 3 Microcontroladores:
1. **Flashear Cerebro 1:** Carga `cerebro_1_ambiental.cpp` en el ESP32 ambiental configurando la MAC del Cerebro 3.
2. **Flashear Cerebro 2:** Carga `cerebro_2_biometrico.cpp` en el ESP32 biométrico configurando la MAC del Cerebro 3.
3. **Flashear Cerebro 3:** Carga `central_node.cpp` asegurando que `pesos_nasa.h` esté en el mismo directorio de compilación.
4. Conéctate con tu smartphone o tablet a la red WiFi:
   - **SSID:** `HELIOS_NETWORK`
   - **Contraseña:** `HELIOS_PASSWORD`
5. Abre `frontend/index.html` en el navegador del dispositivo móvil. Se sincronizará en tiempo real con el socket del traje espacial (`ws://192.168.4.1/ws`).

---

## 🛡️ 8. Principios de Ingeniería de Software (Clean Code)

- **Arquitectura Modular (SRP):** Desacoplamiento total entre los sensores ambientales (Cerebro 1), sensores fisiológicos (Cerebro 2) y la unidad de inferencia y red (Cerebro 3).
- **Tipado Estático Riguroso:** El código Python implementa Type Hinting completo (`Tuple`, `pd.DataFrame`, `np.ndarray`, etc.).
- **Trazabilidad Profesional:** Prohibido el uso de `print()` no estructurado; se utiliza el módulo `logging` de Python (niveles INFO y ERROR con captura de excepciones específicas) y macros dedicadas `log_info()` y `log_error()` en C++.
- **Operaciones Vectorizadas en Pandas:** Transformaciones numéricas sin bucles iterativos (`iterrows()` / `itertuples()`).
- **Seguridad en Memoria:** Ausencia de bloqueos por `delay()` mediante el uso de timers y ticks de FreeRTOS (`vTaskDelay`). Eliminación de fragmentación en Heap serializando JSONs directamente en buffers estáticos de Stack.

---

## 📜 9. Licencia

Este proyecto está distribuido bajo la licencia **MIT**. Consulta el archivo `LICENSE` para más información.

---

**Desarrollado con dedicación a la exploración espacial y la computación biomimética al límite.**  
Autor: [@robertpid](https://github.com/robertpid)
