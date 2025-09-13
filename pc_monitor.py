#!/usr/bin/env python3
import subprocess
import os
import time
import json
import psutil
import serial
from datetime import datetime
import getpass

REFRESH_RATE = 2 # Intervalo do envio de dados (2 segundos é o mais indicado)
SERIAL_PORT = '/dev/ttyUSB0' # A porta aonde o ESP32 está conectado
BAUD_RATE = 115200 # Velocidade da conexão

scroll_positions = {}

def get_cpu_temp():
    # Obtém temperatura real da CPU por package ou média dos núcleos
    # executando sensors via subprocess procurando a linha packge id 0 para pegar a temperatura média, caso não encontre
    # excuta um fallback lendo direto de /temp1_input se nada funcionar retorna 0.0
    try:
        result = subprocess.run(['sensors'], capture_output=True, text=True, timeout=2)
        temps = []
        package_temp = None

        for line in result.stdout.splitlines():
            if "Package id 0" in line and '°C' in line:
                package_temp = float(line.split('+')[1].split('°C')[0].strip())
            elif "Core " in line and '°C' in line:
                core_temp = float(line.split('+')[1].split('°C')[0].strip())
                temps.append(core_temp)

        if package_temp:
            return package_temp
        elif temps:
            return sum(temps) / len(temps)
    except Exception:
        pass

    # fallback: tenta sysfs
    try:
        for hwmon in range(10):
            path = f'/sys/class/hwmon/hwmon{hwmon}/temp1_input'
            if os.path.exists(path):
                with open(path) as f:
                    return int(f.read().strip()) / 1000.0
    except Exception:
        pass

    return 0.0


def get_gpu_usage():
    # executa o radeontop e procura por um linha contendo gpu e % extrai a porcentagem de uso da GPU
    # Caso não encontre retorna 0.0
    try:
        result = subprocess.run(['radeontop', '-d', '-', '-l', '1'],
                                capture_output=True, text=True, timeout=2)
        for line in result.stdout.splitlines():
            if 'gpu' in line.lower() and '%' in line:
                return float(line.split('%')[0].split()[-1])
    except:
        pass
    return 0.0

def get_gpu_temp():

    # Obtém temperatura da GPU edge/junction/mem
    # Executa novamente o sensors localiza o bloco refetente o amdgpu-pci e exatraia temperatura de difetentes sensores são ele
    # edge, junction e mem. Da prioridade ao edge, depois junctione depois mem. se falha tenta via hwmon2/temp1_input
    # caso os dois metodos falhe retorna 0.0 

    try:
        result = subprocess.run(['sensors'], capture_output=True, text=True, timeout=2)
        gpu_temps = {}

        capture = False
        for line in result.stdout.splitlines():
            if "amdgpu-pci" in line:
                capture = True
                continue
            if capture:
                if not line.strip():  # fim do bloco
                    break
                if "edge:" in line:
                    gpu_temps["edge"] = float(line.split('+')[1].split('°C')[0].strip())
                elif "junction:" in line:
                    gpu_temps["junction"] = float(line.split('+')[1].split('°C')[0].strip())
                elif "mem:" in line:
                    gpu_temps["mem"] = float(line.split('+')[1].split('°C')[0].strip())

        # Prioridade: edge > junction > mem
        if "edge" in gpu_temps:
            return gpu_temps["edge"]
        elif "junction" in gpu_temps:
            return gpu_temps["junction"]
        elif "mem" in gpu_temps:
            return gpu_temps["mem"]
    except Exception:
        pass

    # fallback sysfs
    try:
        temp_path = "/sys/class/hwmon/hwmon2/temp1_input"
        if os.path.exists(temp_path):
            with open(temp_path, "r") as f:
                return int(f.read().strip()) / 1000.0
    except Exception:
        pass

    return 0.0

def get_disk_usage():

    # Usa psutil.disk_usage('/') para medir o uso do disco principal

    try:
        disk = psutil.disk_usage('/')
        return {'percent': disk.percent, 'used': disk.used // (1024**3), 'total': disk.total // (1024**3)}
    except:
        return {'percent': 0, 'used': 0, 'total': 0}

def get_scrolling_text(text, max_len=28, key="media"):

    # Gera um texto rolante para o causar um efeito no display

    if key not in scroll_positions:
        scroll_positions[key] = 0
    if len(text) <= max_len:
        scroll_positions[key] = 0
        return text
    padded = text + "   " + text
    pos = scroll_positions[key]
    display = padded[pos:pos+max_len]
    scroll_positions[key] = (pos + 1) % (len(text)+3)
    return display

def get_system_info():
    # Pegando as informações do sistema como Kernel, horario...
    # Resumindo junta todas as informações do sistema
    try:
        cpu_usage = psutil.cpu_percent(interval=0.5)
        cpu_temp = get_cpu_temp()
        gpu_usage = get_gpu_usage()
        gpu_temp = get_gpu_temp()
        mem = psutil.virtual_memory()
        disk = get_disk_usage()
        now = datetime.now().strftime("%H:%M:%S")
        user = getpass.getuser()
        hostname = os.uname().nodename
        kernel = os.uname().release

        return {
            'cpu': cpu_usage,
            'cpu_temp': cpu_temp,
            'gpu': {'usage': gpu_usage, 'temp': gpu_temp},
            'ram': {'percent': mem.percent, 'used': mem.used // (1024**3), 'total': mem.total // (1024**3)},
            'disk': disk,
            'time': now,
            'system': {'user': user, 'hostname': hostname, 'kernel': kernel}
        }
    except:
        return None

def main():

    # Abre a porta serial com o ESP32, aguarda 2 segundo para estabilizar a conexão
    # Loop infinito, chama o get_system_info() converte os dados em json com json.dumps envia pela porta serial
    # Aguarda o REFFRESH_RATE.
    # Garante que porta vai ser fechada antes de encerrar

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        time.sleep(2)
        print(f"Conectado à {SERIAL_PORT}")

        while True:
            data = get_system_info()
            if data:
                ser.write((json.dumps(data) + '\n').encode())
            time.sleep(REFRESH_RATE)

    except KeyboardInterrupt:
        print("Interrompido")
    finally:
        if 'ser' in locals():
            ser.close()

# Gatente que só sera executado quando for rodado direamente e não quando for importado como módulo.        
if __name__ == "__main__":
    main()
