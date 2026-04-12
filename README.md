# 👁️ Projeto ARGUS: Ecossistema IoT Assistivo

![INTERFACE]([https://img.shields.io/badge/Status-Em_Desenvolvimento-brightgreen](https://www.youtube.com/watch?v=AZ03ZFNZcRM&t=1s))
![APP]([https://img.shields.io/badge/Plataforma-ESP32-blue](https://www.youtube.com/watch?v=AZ03ZFNZcRM&t=1s))
![IoT]([https://img.shields.io/badge/Tecnologia-IoT_%7C_Edge_Computing-orange](https://www.youtube.com/watch?v=H1biLwCZ6vE&t=3s))

## 📌 Descrição do Projeto

O **Projeto ARGUS** é um ecossistema IoT assistivo focado em fornecer Inteligência Artificial, monitoramento sensorial e autonomia. 

Originalmente concebido para auxiliar deficientes visuais e idosos, a arquitetura modular e de baixo custo do ARGUS permite sua expansão para monitoramento de rotinas em hospitais, Unidades Básicas de Saúde (UBS) e na detecção de anomalias em ambientes residenciais (Smart Homes).

O sistema atua de forma proativa através de processamento local (**Edge Computing**) para fornecer monitoramento em $360^{\circ}$, emitindo avisos sonoros imediatos e alertas para familiares ou equipes médicas em situações de risco.

---

## 🏥 Casos de Uso e Aplicações

*   👵 **Idosos e Deficientes Visuais:** Prevenção de acidentes domésticos, lembretes de medicação e alertas de emergência (quedas, vazamento de gás).
*   🏥 **Hospitais e UBS:** Monitoramento ambiental de leitos e salas de medicação, controle de acesso a pacientes via RFID e detecção de pedidos de socorro por voz.
*   🏠 **Residências (Smart Home):** Detecção de intrusos, monitoramento contínuo da qualidade do ar/vazamentos e análise comportamental contínua.

---

## ⚙️ Hardware e Componentes

O ARGUS utiliza hardware de baixo custo focado em alta eficiência energética e de processamento:

*   🧠 **ESP32 Core:** Microcontrolador central com Wi-Fi, Bluetooth e processamento de IA local.
*   📷 **ESP32-CAM:** Monitoramento visual e detecção de padrões em eventos críticos.
*   🎤 **Sensor de Som:** Detecta ruídos anômalos, impactos (possíveis quedas) e pedidos de socorro.
*   🚶 **Sensor de Presença (PIR):** Identifica movimentação em zonas de perigo (escadas, fogão, áreas restritas).
*   ☁️ **Sensor MQ135:** Análise de gases tóxicos e qualidade do ar (Amônia, CO2, etc).
*   🌡️ **Sensor DHT22:** Monitoramento de temperatura e umidade para prevenção de estresse térmico.
*   🏷️ **Módulo RFID/NFC:** Leitura de dados médicos e validação de medicação do usuário.

---

## 🧠 IA Assistiva e Respostas Proativas

O sistema processa informações localmente (Edge) e executa interações por voz dependendo do cenário. Exemplos de atuação:

🔹 **Quedas / Impactos**
Ao detectar ruídos altos, o sistema questiona:
> 🔊 *"Detectei um barulho alto. Você está bem?"*
*(Se o usuário não confirmar estado positivo em 10 segundos, o alarme é acionado e contatos são notificados via SMS, Email, Telegram ou Nuvem).*

🔹 **Zonas de Perigo**
Alerta de proximidade em escadas ou áreas restritas:
> 🔊 *"Cuidado! Você está perto de um local perigoso."*

🔹 **Gás Tóxico**
Alerta de evacuação imediata:
> 🔊 *"ALERTA! Cheiro de gás. Saia do ambiente agora!"*

🔹 **Controle de Medicação**
Identificação via RFID com confirmação:
> 🔊 *"Remédio identificado. Uso registrado com sucesso."*

---

## 📡 Resiliência Offline e Sincronização

O ecossistema ARGUS foi projetado para **não depender inteiramente da internet**, garantindo segurança contínua:

*   🔋 **Modo Offline:** Na ausência de conexão, a IA local e os alarmes sonoros continuam operando normalmente. Todos os dados críticos são salvos localmente em um módulo de cartão SD.
*   🔄 **Sincronização Cloud:** Assim que a rede é reestabelecida, o sistema sincroniza os logs pendentes com o Dashboard online e envia os relatórios diários gerados no período offline.

---

---



## YOLOv8 Live Object Detection Android Application

### Description
This Android application is designed to perform live object detection using the YOLOv8 machine learning model. YOLOv8 (You Only Look Once version 8) is known for its real-time object detection capabilities, and this app brings that functionality to Android devices.

### Getting Started
To use this repository for any custom YOLOv8 Object detection model, follow these steps:
1. Clone this repository to your local machine.
2. Put your .tflite model inside the assets folder
3. Rename paths of your model in Constants.kt file
4. **Build and Run:**

### Contributing
Contributions are welcome! If you want to contribute to this project, feel free to fork the repository and submit a pull request with your changes.

### Contact
For any questions or feedback, feel free to open an [issue](https://github.com/surendramaran/Machine-Learning-in-Mobile/issues/new) in the repository.

### Support
If you find this project helpful and want to support its development, consider becoming a patron on [Patreon](https://www.patreon.com/SurendraMaran). Your support will help in maintaining and improving the project. Thank you!

## 📊 Estrutura de Dados e Dashboard

O banco de dados do sistema registra eventos detalhados para análise histórica e treinamento de futuras IAs. 

**Exemplo de Payload de Dados:**
```json
{
  "timestamp": "2023-10-25T14:30:00Z",
  "device_id": "ARGUS_NODE_01",
  "sensor_ativado": "Som",
  "valor_detectado": "85dB",
  "tipo_evento": "Risco Crítico",
  "user_id": "USR_045",
  "contato_notificado": "+5511999999999"
}



