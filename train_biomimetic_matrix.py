"""
Script de Entrenamiento Biomimético - NASA EVA Telemetry (Protocolo NEWS2/HELIOS)
Autor: Lead Data Scientist
Descripción: Ingiere telemetría sintética (basada en umbrales médicos) y entrena 
una Red Neuronal Profunda (Multicapa) exportando la arquitectura a C++.
"""

import pandas as pd
import numpy as np
import logging
from typing import Tuple

logging.basicConfig(level=logging.INFO, format="%(asctime)s - [%(levelname)s] - %(message)s")

def generate_synthetic_nasa_telemetry(num_samples: int = 5000) -> Tuple[pd.DataFrame, np.ndarray]:
    logging.info(f"Generando {num_samples} muestras sintéticas H.E.L.I.O.S. con etiquetas de anomalía.")
    try:
        np.random.seed(101)
        data = {
            'spo2': np.random.normal(98, 1.5, num_samples),          
            'bpm': np.random.normal(70, 10, num_samples),            
            'body_temp': np.random.normal(36.6, 0.3, num_samples),   
            'ecg': np.random.normal(0, 0.1, num_samples),            
            'internal_temp': np.random.normal(22.5, 1.5, num_samples),
            'humidity': np.random.normal(50, 5, num_samples),        
            'air_quality': np.random.normal(0, 5, num_samples),      
            'radiation': np.random.exponential(1, num_samples)       
        }
        df = pd.DataFrame(data)
        # Riesgo Crítico (Anomalías Multietiqueta: 20 Diagnósticos)
        labels = np.zeros((num_samples, 20))
        
        # 0: Hipoxia
        hypoxia_idx = np.random.choice(num_samples, size=int(num_samples * 0.05), replace=False)
        labels[hypoxia_idx, 0] = 1.0
        df.loc[hypoxia_idx, 'spo2'] = np.random.uniform(80, 89, len(hypoxia_idx))
        
        # 1: Estrés Cardíaco
        cardiac_idx = np.random.choice(num_samples, size=int(num_samples * 0.05), replace=False)
        labels[cardiac_idx, 1] = 1.0
        df.loc[cardiac_idx, 'bpm'] = np.random.uniform(130, 180, len(cardiac_idx))
        df.loc[cardiac_idx, 'ecg'] = np.random.uniform(0.5, 1.0, len(cardiac_idx))
        
        # 2: Peligro Ambiental
        env_idx = np.random.choice(num_samples, size=int(num_samples * 0.05), replace=False)
        labels[env_idx, 2] = 1.0
        df.loc[env_idx, 'radiation'] = np.random.uniform(8, 15, len(env_idx))
        df.loc[env_idx, 'air_quality'] = np.random.uniform(80, 120, len(env_idx))
        
        # Diagnósticos Avanzados (3 al 19) - Simulando anomalías espaciales complejas
        for i in range(3, 20):
            idx = np.random.choice(num_samples, size=int(num_samples * 0.02), replace=False)
            labels[idx, i] = 1.0
            # Perturbaciones cruzadas para forzar Sparse Coding en las neuronas ocultas
            df.loc[idx, 'body_temp'] = np.random.uniform(38.0, 41.5, len(idx))
            df.loc[idx, 'internal_temp'] = np.random.uniform(30.0, 45.0, len(idx))
            df.loc[idx, 'humidity'] = np.random.uniform(10, 30, len(idx))
        
        return df, labels
    except (ValueError, KeyError) as e:
        logging.error(f"Fallo al generar dataset: {e}")
        return pd.DataFrame(), np.array([])

def clean_and_normalize_telemetry(raw_data: pd.DataFrame) -> Tuple[pd.DataFrame, pd.Series, pd.Series]:
    logging.info("Iniciando normalización del dataset.")
    try:
        cleaned = raw_data.replace([np.inf, -np.inf], np.nan).dropna()
        min_vals = cleaned.min()
        max_vals = cleaned.max()
        normalized = (cleaned - min_vals) / (max_vals - min_vals + 1e-8)
        return normalized, min_vals, max_vals
    except (ValueError, TypeError) as e:
        logging.error(f"Error normalizando: {e}")
        return pd.DataFrame(), pd.Series(dtype=float), pd.Series(dtype=float)

def train_deep_neural_network(data: pd.DataFrame, target: np.ndarray, epochs: int = 2000, learning_rate: float = 0.5) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    X = data.to_numpy()
    Y = target # target es (N, 20)
    
    input_size = X.shape[1]
    hidden_size = 64  # Kenyon Cells ultra-densas para procesar 20 salidas
    output_size = 20  # 20 Diagnósticos independientes
    
    np.random.seed(42)
    # Inicialización de pesos (He/Xavier)
    W1 = np.random.randn(input_size, hidden_size) * np.sqrt(2. / input_size)
    b1 = np.zeros((1, hidden_size))
    W2 = np.random.randn(hidden_size, output_size) * np.sqrt(2. / hidden_size)
    b2 = np.zeros((1, output_size))
    
    logging.info(f"Entrenando Red Neuronal Profunda. Topología: [{input_size} -> {hidden_size} -> {output_size}]")
    
    for epoch in range(epochs):
        # Forward pass
        Z1 = np.dot(X, W1) + b1
        A1 = np.maximum(0, Z1) # Activación ReLU
        Z2 = np.dot(A1, W2) + b2
        A2 = 1 / (1 + np.exp(-np.clip(Z2, -250, 250))) # Activación Sigmoide
        
        # Backward pass
        dZ2 = A2 - Y
        dW2 = np.dot(A1.T, dZ2) / X.shape[0]
        db2 = np.sum(dZ2, axis=0, keepdims=True) / X.shape[0]
        
        dA1 = np.dot(dZ2, W2.T)
        dZ1 = dA1 * (Z1 > 0)
        dW1 = np.dot(X.T, dZ1) / X.shape[0]
        db1 = np.sum(dZ1, axis=0, keepdims=True) / X.shape[0]
        
        W1 -= learning_rate * dW1
        b1 -= learning_rate * db1
        W2 -= learning_rate * dW2
        b2 -= learning_rate * db2
            
    logging.info("Entrenamiento multicapa completado.")
    return W1, b1, W2, b2

def export_dnn_to_cpp_header(W1: np.ndarray, b1: np.ndarray, W2: np.ndarray, b2: np.ndarray, min_vals: pd.Series, max_vals: pd.Series, filepath: str) -> None:
    logging.info(f"Exportando sub-niveles de la red a {filepath}")
    try:
        with open(filepath, 'w') as f:
            f.write("#ifndef PESOS_NASA_H\n#define PESOS_NASA_H\n\n")
            f.write("/* Red Neuronal Profunda H.E.L.I.O.S. (Multicapa) */\n\n")
            
            # Normalización
            f.write(f"const float MIN_VALS[{len(min_vals)}] = {{")
            f.write(", ".join([f"{v:.6f}" for v in min_vals]))
            f.write("};\n")
            
            f.write(f"const float MAX_VALS[{len(max_vals)}] = {{")
            f.write(", ".join([f"{v:.6f}" for v in max_vals]))
            f.write("};\n\n")
            
            # Capa 1
            f.write(f"const int L1_ROWS = {W1.shape[0]};\n")
            f.write(f"const int L1_COLS = {W1.shape[1]};\n")
            f.write(f"const float W1[{W1.shape[0]}][{W1.shape[1]}] = {{\n")
            for i in range(W1.shape[0]):
                row_str = ", ".join([f"{w:.6f}" for w in W1[i]])
                f.write(f"    {{{row_str}}}" + (",\n" if i < W1.shape[0]-1 else "\n"))
            f.write("};\n")
            
            b1_str = ", ".join([f"{b:.6f}" for b in b1[0]])
            f.write(f"const float b1[{b1.shape[1]}] = {{{b1_str}}};\n\n")
            
            # Capa 2
            f.write(f"const int L2_ROWS = {W2.shape[0]};\n")
            f.write(f"const int L2_COLS = {W2.shape[1]};\n")
            f.write(f"const float W2[{W2.shape[0]}][{W2.shape[1]}] = {{\n")
            for i in range(W2.shape[0]):
                row_str = ", ".join([f"{w:.6f}" for w in W2[i]])
                f.write(f"    {{{row_str}}}" + (",\n" if i < W2.shape[0]-1 else "\n"))
            f.write("};\n")
            
            b2_str = ", ".join([f"{b:.6f}" for b in b2[0]])
            f.write(f"const float b2[{b2.shape[1]}] = {{{b2_str}}};\n\n")
            
            f.write("#endif\n")
        logging.info("Topología profunda exportada exitosamente a C++.")
    except IOError as e:
        logging.error(f"Fallo de E/S: {e}")

if __name__ == "__main__":
    df, labels = generate_synthetic_nasa_telemetry()
    norm_df, min_vals, max_vals = clean_and_normalize_telemetry(df)
    if not norm_df.empty:
        W1, b1, W2, b2 = train_deep_neural_network(norm_df, labels)
        export_dnn_to_cpp_header(W1, b1, W2, b2, min_vals, max_vals, "pesos_nasa.h")
