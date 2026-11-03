import os
import pickle
import zipfile
import urllib.request
import numpy as np
import m2cgen as m2c
from sklearn.svm import LinearSVC
from sklearn.pipeline import make_pipeline
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, accuracy_score

DATA_DIR = "./WESAD"

# ==========================================
# 1. PROCESSAMENTO E EXTRAÇÃO DE FEATURES
# ==========================================
SUBJECTS = [f'S{i}' for i in range(2, 18) if i != 12] # S12 é omitido no dataset original

# Frequências de amostragem no dispositivo de pulso (Empatica E4)
FS_PPG = 64
FS_ACC = 32
FS_LABEL = 700

# Janela de 60 segundos com pulo (overlap) de 5 segundos
WINDOW_SEC = 60
STEP_SEC = 5

X = []
y = []

print("Extraindo features de todos os participantes...")

for subject in SUBJECTS:
    subject_file = os.path.join(DATA_DIR, subject, f"{subject}.pkl")
    if not os.path.exists(subject_file):
        continue

    with open(subject_file, 'rb') as f:
        data = pickle.load(f, encoding='latin1')

    wrist_ppg = data['signal']['wrist']['BVP'].flatten()
    wrist_acc = data['signal']['wrist']['ACC']
    labels = data['label']

    # Tamanho das janelas em número de amostras
    win_ppg = WINDOW_SEC * FS_PPG
    step_ppg = STEP_SEC * FS_PPG
    
    win_acc = WINDOW_SEC * FS_ACC
    step_acc = STEP_SEC * FS_ACC
    
    win_lbl = WINDOW_SEC * FS_LABEL
    step_lbl = STEP_SEC * FS_LABEL

    n_windows = (len(wrist_ppg) - win_ppg) // step_ppg

    for i in range(n_windows):
        # Índices das janelas
        idx_ppg = i * step_ppg
        idx_acc = i * step_acc
        idx_lbl = i * step_lbl

        # Corte dos sinais
        ppg_win = wrist_ppg[idx_ppg : idx_ppg + win_ppg]
        acc_win = wrist_acc[idx_acc : idx_acc + win_acc]
        lbl_win = labels[idx_lbl : idx_lbl + win_lbl]

        # Determina a classe predominante na janela (1: Baseline, 2: Estresse)
        counts = np.bincount(lbl_win)
        most_frequent_label = np.argmax(counts)

        # Filtra apenas Baseline (1) e Estresse (2)
        if most_frequent_label in [1, 2]:
            # Feature do Acelerômetro: Magnitude Tridimensional
            acc_mag = np.sqrt(np.sum(acc_win**2, axis=1))
            acc_mean = np.mean(acc_mag)
            acc_std = np.std(acc_mag)

            # Features do PPG no domínio do tempo
            ppg_mean = np.mean(ppg_win)
            ppg_std = np.std(ppg_win)

            # Vetor de features: [PPG_mean, PPG_std, ACC_mean, ACC_std]
            X.append([ppg_mean, ppg_std, acc_mean, acc_std])
            # Mapeamento: Baseline (1 -> Class 0), Estresse (2 -> Class 1)
            y.append(0 if most_frequent_label == 1 else 1)

X = np.array(X)
y = np.array(y)

print(f"\nExtração concluída!")
print(f"Total de amostras geradas: {X.shape[0]}")
print(f"Classe 0 (Repouso/Baseline): {np.sum(y == 0)}")
print(f"Classe 1 (Estresse): {np.sum(y == 1)}")

# ==========================================
# 2. DIVISÃO 80% TREINO / 20% TESTE
# ==========================================
# test_size=0.20 garante os 20% para teste e 80% para treino.
# stratify=y mantém a proporção exata de amostras de estresse e repouso em ambos os conjuntos.
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.20, random_state=42, stratify=y
)

print(f"\nDivisão do Dataset:")
print(f"Amostras de Treino (80%): {X_train.shape[0]}")
print(f"Amostras de Teste  (20%): {X_test.shape[0]}")

# ==========================================
# 3. TREINAMENTO SEPARADO (SCALER + SVC)
# ==========================================
print("\nTreinando o modelo LinearSVC...")

scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# LinearSVC não usa vetores de suporte internos -> m2cgen exporta sem erro de recursão
clf = LinearSVC(C=1.0, class_weight='balanced', max_iter=10000, random_state=42)
clf.fit(X_train_scaled, y_train)

# Avaliação no conjunto de teste (20%)
y_pred = clf.predict(X_test_scaled)

print("\n--- Resultados de Validação (Conjunto de Teste - 20%) ---")
print(f"Acurácia Geral: {accuracy_score(y_test, y_pred) * 100:.2f}%\n")
print(classification_report(y_test, y_pred, target_names=['Repouso', 'Estresse']))

# ==========================================
# 4. GERANDO ARQUIVO MODEL_WESAD.H COM M2CGEN
# ==========================================
print("Exportando modelo para C/C++ usando m2cgen...")

# O m2cgen exportará uma simples equação linear sem estourar a recursão
code_c = m2c.export_to_c(clf)

mean_str = ", ".join(map(str, scaler.mean_))
scale_str = ", ".join(map(str, scaler.scale_))

header_content = f"""#ifndef MODEL_WESAD_H
#define MODEL_WESAD_H

#ifdef __cplusplus
extern "C" {{
#endif

// Parâmetros de Normalização (StandardScaler)
static const double SCALER_MEAN[4] = {{ {mean_str} }};
static const double SCALER_SCALE[4] = {{ {scale_str} }};

/*
  Código C gerado pelo m2cgen para o Linear SVM
*/
{code_c}

/*
  Função utilitária que aplica a normalização e executa a inferência
  Entrada: double features[4] -> [ppg_mean, ppg_std, acc_mean, acc_std]
  Retorno: int (0 = Repouso, 1 = Estresse)
*/
static inline int predict_wesad_stress(const double features[4]) {{
    double normalized_input[4];
    for (int i = 0; i < 4; i++) {{
        normalized_input[i] = (features[i] - SCALER_MEAN[i]) / SCALER_SCALE[i];
    }}
    double res = score(normalized_input);
    return (res > 0.0) ? 1 : 0;
}}

#ifdef __cplusplus
}}
#endif

#endif // MODEL_WESAD_H
"""

with open("model_wesad.h", "w") as f:
    f.write(header_content)

print("Arquivo 'model_wesad.h' gerado com sucesso!")
