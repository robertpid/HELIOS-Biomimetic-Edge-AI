/**
 * H.E.L.I.O.S. HUD Front-End Logic
 * Sin dependencias externas (Vanilla JS) para asegurar operatividad offline.
 */

// Elementos DOM
const wsStatusDot = document.getElementById('ws-status-dot');
const wsStatusText = document.getElementById('ws-status-text');
const latencyVal = document.getElementById('latency-val');

const vSpO2 = document.getElementById('val-spo2');
const vBPM = document.getElementById('val-bpm');
const vBTemp = document.getElementById('val-btemp');
const vITemp = document.getElementById('val-itemp');
const vHum = document.getElementById('val-hum');
const vRad = document.getElementById('val-rad');

const anomalyBar = document.getElementById('anomaly-bar');
const anomalyText = document.getElementById('anomaly-text');
const aiCoreCard = document.querySelector('.ai-core-card');

const diagBox = document.getElementById('diagnostic-box');
const diagStatus = document.getElementById('diag-status');
const diagMessage = document.getElementById('diag-message');
const actionList = document.getElementById('action-list');

// Configuración del ECG (Osciloscopio Custom en Canvas)
const canvas = document.getElementById('ecgCanvas');
const ctx = canvas.getContext('2d');
let ecgData = new Array(150).fill(0); // Buffer de 150 puntos

// Conexión WebSocket
let ws;
let lastPing = 0;
// La IP por defecto del Access Point del ESP32
const gateway = `ws://192.168.4.1/ws`; 

function connectWebSocket() {
    console.log("Intentando conectar al Cerebro 3 en: " + gateway);
    try {
        ws = new WebSocket(gateway);
    } catch(e) {
        console.warn("WebSocket no disponible, activando modo simulación...");
        return; 
    }
    
    ws.onopen = () => {
        wsStatusDot.classList.add('connected');
        wsStatusText.innerText = 'CONECTADO AL TRAJE';
        wsStatusText.style.color = 'var(--cyan)';
    };
    
    ws.onclose = () => {
        wsStatusDot.classList.remove('connected');
        wsStatusText.innerText = 'SEÑAL PERDIDA';
        wsStatusText.style.color = 'var(--red)';
        setTimeout(connectWebSocket, 2000); // Intento de reconexión automática
    };
    
    ws.onmessage = (event) => {
        const now = Date.now();
        if(lastPing !== 0) latencyVal.innerText = now - lastPing;
        lastPing = now;
        
        try {
            const data = JSON.parse(event.data);
            updateDashboard(data);
        } catch(e) {
            console.error("Error parseando telemetría", e);
        }
    };
}

// ----------------------------------------------------
// MOTOR DE ACTUALIZACIÓN DOM
// ----------------------------------------------------
function updateDashboard(data) {
    // 1. Refrescar Biometría
    vSpO2.innerText = data.spo2.toFixed(1);
    vBPM.innerText = data.bpm.toFixed(0);
    vBTemp.innerText = data.body_temp.toFixed(1);
    vITemp.innerText = data.internal_temp.toFixed(1);
    vHum.innerText = data.humidity.toFixed(1);
    vRad.innerText = data.radiation.toFixed(2);
    
    // 2. Refrescar Buffer del ECG
    ecgData.push(data.ecg);
    if(ecgData.length > 150) ecgData.shift();
    drawECG();

    // 3. Predicción de la Red Neuronal (Deep Learning)
    const score = (data.anomaly_score * 100);
    
    // Si el score es -1, significa Quality Flag Inválido
    if(data.anomaly_score < 0) {
        anomalyBar.style.width = "0%";
        anomalyText.innerText = "N/A";
    } else {
        anomalyBar.style.width = score + "%";
        anomalyText.innerText = score.toFixed(2) + "%";
    }
    
    // 4. Lógica de Diagnóstico y Sugerencias
    runDiagnostics(data, score);
}

// ----------------------------------------------------
// MOTOR AUTÓNOMO DE DIAGNÓSTICOS (Sugerencias Tácticas)
// ----------------------------------------------------
function runDiagnostics(data, score) {
    let anomalies = [];
    let actions = [];
    
    // Reglas Clínicas basadas en Protocolo NEWS2
    if (data.spo2 < 92) anomalies.push("Hipoxia Crítica");
    if (data.bpm > 130) anomalies.push("Taquicardia Severa");
    if (data.body_temp > 38.5) anomalies.push("Hipertermia");
    if (data.radiation > 8.0) anomalies.push("Peligro Radiación UV");

    // Verificar Flags de Calidad de Hardware
    if (!data.sensor_valid || data.anomaly_score < 0) {
        aiCoreCard.classList.add('alert-state');
        anomalyBar.style.background = 'var(--red)';
        anomalyText.style.color = 'var(--red)';
        
        diagBox.classList.add('danger');
        diagStatus.innerText = "FALLO HARDWARE";
        diagStatus.style.color = "var(--red)";
        diagMessage.innerText = "Los sensores han perdido acoplamiento físico con el astronauta. IA Supresa.";
        
        actions.push("Detener actividad de inmediato.");
        actions.push("Reajustar cableado de electrodos (Cerebro 2).");
        actionList.classList.add('danger');
        
    } else if (score > 75.0 || anomalies.length > 0) {
        // ESTADO DE EMERGENCIA DECLARADO POR LA RED NEURONAL
        aiCoreCard.classList.add('alert-state');
        anomalyBar.style.background = 'var(--red)';
        anomalyText.style.color = 'var(--red)';
        
        diagBox.classList.add('danger');
        diagStatus.innerText = "ALERTA CRÍTICA";
        diagStatus.style.color = "var(--red)";
        diagMessage.innerText = "La Red Neuronal ha detectado un colapso inminente: " + anomalies.join(", ");
        
        // Sugerencias Dinámicas
        if(data.radiation > 8.0) actions.push("ABORTAR EVA. Buscar blindaje inmediatamente en módulo central.");
        if(data.spo2 < 92) actions.push("Aumentar flujo de O2 primario del traje al 100%.");
        if(data.bpm > 130) actions.push("Iniciar protocolo de reposo estricto. Respiración controlada.");
        actionList.classList.add('danger');
        
    } else {
        // ESTADO NOMINAL (SEGURO)
        aiCoreCard.classList.remove('alert-state');
        anomalyBar.style.background = 'var(--cyan)';
        anomalyText.style.color = 'var(--cyan)';
        
        diagBox.classList.remove('danger');
        diagStatus.innerText = "NOMINAL";
        diagStatus.style.color = "var(--cyan)";
        diagMessage.innerText = "Todos los parámetros fisiológicos y ambientales están dentro de los límites operativos seguros.";
        
        actions.push("Continuar misión EVA de acuerdo al cronograma.");
        actionList.classList.remove('danger');
    }
    
    // Renderizar la lista de acciones recomendadas
    actionList.innerHTML = "";
    actions.forEach(act => {
        const li = document.createElement('li');
        li.innerText = act;
        actionList.appendChild(li);
    });
}

// ----------------------------------------------------
// RENDERIZADO GRÁFICO (Osciloscopio ECG)
// ----------------------------------------------------
function drawECG() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.beginPath();
    ctx.strokeStyle = '#00f3ff';
    ctx.lineWidth = 2;
    ctx.shadowBlur = 5;
    ctx.shadowColor = '#00f3ff';
    
    const sliceWidth = canvas.width / ecgData.length;
    let x = 0;
    
    for(let i = 0; i < ecgData.length; i++) {
        // Mapear el voltaje del sensor (ej. de 0 a 1) al Canvas
        // Multiplicador ajustado para fines visuales
        let y = (canvas.height / 2) - (ecgData[i] * 30); 
        
        if(i === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
        x += sliceWidth;
    }
    ctx.stroke();
}

// ----------------------------------------------------
// INICIALIZACIÓN
// ----------------------------------------------------
connectWebSocket();

// Solo para efectos visuales si no hay ESP32 físico conectado ahora:
// Un simulador que inyecta datos falsos para que veas el diseño
setInterval(() => {
    if(!ws || ws.readyState !== WebSocket.OPEN) {
        let mockData = {
            spo2: 98 + Math.random(),
            bpm: 70 + (Math.random() * 5),
            body_temp: 36.6 + (Math.random() * 0.2),
            ecg: Math.sin(Date.now() / 150) + (Math.random() * 0.2), // Falso latido
            internal_temp: 22.5,
            humidity: 50,
            air_quality: Math.random() * 2,
            radiation: Math.random(),
            sensor_valid: true,
            anomaly_score: 0.05
        };
        
        // Simular un infarto espontáneo al 5% de probabilidad para probar la UI
        if (Math.random() > 0.95) {
            mockData.bpm = 145;
            mockData.spo2 = 88;
            mockData.anomaly_score = 0.98; // 98% riesgo
        }
        
        // Simular sensor desconectado
        if (Math.random() > 0.98) mockData.sensor_valid = false;

        updateDashboard(mockData);
    }
}, 100);
