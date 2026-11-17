import serial
import csv
import time
import threading

PORTA_SERIAL = "COM3" # Ajuste para sua porta (ex: /dev/ttyUSB0 no Linux)
BAUD_RATE = 115200

estado_real_atual = 0 # Começa em Repouso (0)

def ler_teclado():
    global estado_real_atual
    while True:
        val = input("Digite 0 (Repouso) ou 1 (Estresse) para atualizar o gabarito: ")
        if val in ['0', '1']:
            estado_real_atual = int(val)
            print(f"---> Gabarito atualizado para: {estado_real_atual}")

threading.Thread(target=ler_teclado, daemon=True).start()

ser = serial.Serial(PORTA_SERIAL, BAUD_RATE, timeout=1)

with open("validacao_campo.csv", "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["timestamp", "real", "predito"])
    
    while True:
        linha = ser.readline().decode("utf-8", errors="ignore").strip()
        if linha.startswith(">stress_level:"):
            predito = int(linha.split(":")[1])
            ts = time.strftime("%H:%M:%S")
            writer.writerow([ts, estado_real_atual, predito])
            f.flush()
            print(f"[{ts}] Real: {estado_real_atual} | Predito: {predito}")