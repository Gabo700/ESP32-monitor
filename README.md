# ESP32 - Monitoramento de Sistema

    _____ 
   /     \\ 
  | (•) (•) 
   \\  ^  / 
    |||||  
    -----  

Este projeto utiliza um **ESP32-2432S028** para coletar e exibir informações de um computador em tempo "real", são elas informações de CPU com uso e temperatuda, RAM com uso, Disco com porcentagem de ocupação, GPU uso e temperatura, também exibe informações do user que executa o envio para a porta serial, versão do kernel e hora.  
A comunicação é feita através de um serviço no Arch Linux que envia os dados para o ESP32 via Serial.

---

## Funcionalidades
- Recebe dados de sensores do sistema (temperatura, CPU, memória, etc.).
- Exibe as informações em tempo real em um display conectado ao ESP32.
- Atualização automática e contínua dos dados.

---

### Tecnologias utilizadas
- **ESP32** (C++ / Arduino Framework)  
- **Arch Linux** para coleta de métricas do sistema  
- **Python** (no serviço de envio dos dados)  
- **Display TFT tátil resistivo de 2,8** 

---

![WhatsApp Image 2025-09-13 at 15 04 59(1)](https://github.com/user-attachments/assets/721d5838-d09c-4551-a712-3942413bd957)

![WhatsApp Image 2025-09-13 at 15 12 56](https://github.com/user-attachments/assets/97a8231f-3ee7-4eca-8d3f-cfb7436db9ce)

![WhatsApp Video 2025-09-13 at 10 41 20(1) (online-video-cutter com)](https://github.com/user-attachments/assets/56ee2c20-4158-4fc4-95a8-1f5d552de44c)


