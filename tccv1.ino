// =================================================================================
// ===      PROJETO ARGUS V7.1 - Monitoramento Ambiental e Ficha Médica          ===
// ===      Novidade: Interface Web Dashboard Pro com FOTO e Barra de Carga      ===
// =================================================================================

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 
#include "time.h"

// --- BIBLIOTECAS RFID E SPI ---
#include <SPI.h>
#include <MFRC522.h>

// --- BIBLIOTECA BLUETOOTH ---
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth nao esta habilitado!
#endif
BluetoothSerial SerialBT;

// --- Configurações de Wi-Fi e API ---
const char* ssid = "x";
const char* password = "x";
String writeAPIKey = "x"; 

String latitude = "-x";  
String longitude = "-x"; 

// --- Configurações de Pinos e Display ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- CONFIGURAÇÃO DOS PINOS ---
#define DHT_PIN 4        
#define DHT_TYPE DHT22
#define GAS_SENSOR_PIN 32 
#define PIR_PIN 34       
#define SOUND_PIN 35     

// --- PINOS RFID RC522 ---
#define SS_PIN 5
#define RST_PIN 27
MFRC522 mfrc522(SS_PIN, RST_PIN); 

// --- Objetos ---
DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

// --- VARIÁVEIS DE TEMPO ---
unsigned long t_leitura = 0, t_envio = 0, t_clima = 0, t_troca_tela = 0;
unsigned long t_piscar = 0, t_falar = 0, t_bluetooth = 0; 
unsigned long t_ultimoMovimento = 0, t_ultimoBarulho = 0, t_ultimaTag = 0;

const long INT_LEITURA = 2000;
const long INT_ENVIO = 20000; 
const long INT_CLIMA = 600000; 
const long INT_BLUETOOTH = 10000; 

// --- Variáveis de Sensores ---
float temperatura = 0.0, umidade = 0.0, indiceCalor = 0.0, pontoOrvalho = 0.0;
float valorGasSuavizado = 0.0;
const float FILTRO_ALFA = 0.15; 
String climaDescricao = "Buscando...";
float tempExterna = 0.0;
bool vaiChover = false;

// --- Variáveis de Estado e Perfil ---
int telaAtual = 0;
const int NUM_TELAS = 3;
bool estaPiscando = false;
bool avatarDormindo = false;
bool barulhoDetectado = false;
String falaAtual = "Sistema ARGUS ON...";

// --- DADOS DO PERFIL MÉDICO/PESSOAL ---
String rfidNomeCompleto = "";
String rfidNasc = "";
String rfidTel = "";
String rfidEnd = "";
String rfidSangue = "";
String rfidPCD = "";
String rfidCondicao = "";
String rfidEmergencia = "";

// Declaração da função
void animacaoFichaMedica();

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ARGUS_System"); 
  
  pinMode(PIR_PIN, INPUT);
  pinMode(SOUND_PIN, INPUT);

  Wire.begin(21, 22);
  SPI.begin();
  mfrc522.PCD_Init();
  
  dht.begin();
  analogReadResolution(12);
  analogSetPinAttenuation(GAS_SENSOR_PIN, ADC_11db);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println(F("Falha OLED")); while(1); }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.setCursor(20, 28); display.println("PROJETO ARGUS");
  display.display();
  delay(3000);

  WiFi.begin(ssid, password);
  display.clearDisplay(); display.setCursor(10, 30); display.print("Conectando WiFi..."); display.display();
  
  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) { delay(500); tentativas++; }

  Serial.print("IP do Dashboard ARGUS: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handleRoot); server.begin();
}

void loop() {
  server.handleClient(); 
  unsigned long atual = millis();

  // --- LEITURA DO PIR ---
  if (digitalRead(PIR_PIN) == HIGH) {
    delay(50); 
    if (digitalRead(PIR_PIN) == HIGH) {
      t_ultimoMovimento = atual;
      if (avatarDormindo) {
        avatarDormindo = false;
        falaAtual = "Movimento Detectado!";
        atualizarDisplay();
      }
    }
  } else if (atual - t_ultimoMovimento > 30000) { 
    avatarDormindo = true;
  }

  // --- LEITURA DE SOM ---
  if (digitalRead(SOUND_PIN) == LOW) { 
    delay(20); 
    if (digitalRead(SOUND_PIN) == LOW) {
      barulhoDetectado = true;
      t_ultimoBarulho = atual;
      falaAtual = "Analise acustica...";
      avatarDormindo = false; 
      t_ultimoMovimento = atual;
    }
  }
  if (atual - t_ultimoBarulho > 4000) { barulhoDetectado = false; }

  // --- LEITURA DO RFID (FICHA MÉDICA COMPLETA) ---
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String conteudo = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    conteudo.toUpperCase();
    String uidLido = conteudo.substring(1);
    
    // VERIFICAÇÃO DO CARTÃO
    if (uidLido == "FE E5 3C 05") { // << SEU CÓDIGO AQUI
      
      rfidNomeCompleto = "Mauricio M. D. Oliveira";
      rfidNasc = "21/03/1999";
      rfidTel = "(11) 99999-8888"; // Altere para seu numero
      rfidEnd = "Rua Exemplo, 123, SP"; // Altere para sua rua
      rfidSangue = "O+";
      rfidPCD = "Deficiente Auditivo";
      rfidCondicao = "Hipertenso";
      rfidEmergencia = "Mae: (11) 97777-6666";
      
      if(SerialBT.hasClient()) {
        SerialBT.println("\n=================================");
        SerialBT.println(">>> ARGUS: ALERTA MEDICO <<<");
        SerialBT.println("NOME: " + rfidNomeCompleto);
        SerialBT.println("TIPO SANGUINEO: " + rfidSangue);
        SerialBT.println("PCD: " + rfidPCD);
        SerialBT.println("CONDICAO: " + rfidCondicao);
        SerialBT.println("CONTATO EMERG: " + rfidEmergencia);
        SerialBT.println("=================================\n");
      }
      
      // CHAMA A NOVA ANIMAÇÃO PROFISSIONAL NO OLED
      animacaoFichaMedica();
      
    } else {
      rfidNomeCompleto = "Desconhecido";
    }
    
    falaAtual = "Identidade Confirmada";
    t_ultimaTag = atual;
    avatarDormindo = false;
    telaAtual = 0; 
    t_troca_tela = atual; 
    atualizarDisplay(); 
    mfrc522.PICC_HaltA(); 
  }

  // --- RESTO DAS LÓGICAS ---
  static bool primeiraBusca = true;
  if (WiFi.status() == WL_CONNECTED && (primeiraBusca || atual - t_clima >= INT_CLIMA)) {
    buscarPrevisaoTempoGratuita(); t_clima = atual; primeiraBusca = false;
  }
  
  if (atual - t_piscar > random(2000, 6000)) {
    estaPiscando = true; atualizarDisplay(); delay(random(150, 300)); estaPiscando = false; t_piscar = atual;
  }

  if (atual - t_falar >= 6000 && !avatarDormindo && rfidNomeCompleto == "" && !barulhoDetectado) {
    gerarFalaDoAvatar(); t_falar = atual; atualizarDisplay();
  }

  if (atual - t_leitura >= INT_LEITURA) { lerSensores(); atualizarDisplay(); t_leitura = atual; }
  if (WiFi.status() == WL_CONNECTED && atual - t_envio >= INT_ENVIO) { enviarThingSpeak(); t_envio = atual; }
  if (atual - t_bluetooth >= INT_BLUETOOTH) { enviarDadosBluetooth(); t_bluetooth = atual; }
  
  unsigned long tempoDeExibicao = (telaAtual == 0) ? 15000 : 5000;
  if (atual - t_troca_tela >= tempoDeExibicao) { 
    telaAtual = (telaAtual + 1) % NUM_TELAS; 
    if(telaAtual == 0) rfidNomeCompleto = ""; 
    atualizarDisplay(); 
    t_troca_tela = atual; 
  }
}

// ================= ANIMAÇÃO FICHA MÉDICA OLED =================
void animacaoFichaMedica() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 1. Animação de "Buscando no Banco de Dados"
  display.setCursor(15, 20); display.print("Acessando Argus DB");
  display.drawRect(14, 35, 100, 10, SSD1306_WHITE);
  for(int i = 0; i <= 96; i += 8) {
    display.fillRect(16, 37, i, 6, SSD1306_WHITE);
    display.display();
    delay(50);
  }
  delay(300);

  // 2. Apresentação dos Dados
  String linhas[] = {
    "NOME: Mauricio O.",
    "NASC: 21/03/1999",
    "SANGUE: " + rfidSangue,
    "PCD: Def. Auditivo",
    "COND.: Hipertenso",
    "EMERG: " + rfidEmergencia
  };
  
  display.clearDisplay();
  for(int i = 0; i < 6; i++) {
    display.setCursor(0, i * 10 + 2); 
    for(int j = 0; j < linhas[i].length(); j++) {
      display.print(linhas[i][j]);
      display.display();
      delay(20); 
    }
    delay(200); 
  }
  delay(6000); 
}

// ================= FUNÇÕES DE TELA E LÓGICA =================
void atualizarDisplay() {
  display.clearDisplay();
  if (avatarDormindo) { drawScreenDormindo(); } 
  else {
    switch (telaAtual) {
      case 0: drawScreenAvatar(); break;    
      case 1: drawScreenSensores(); break;  
      case 2: drawScreenClima(); break;  
    }
  }
  display.display();
}

void drawScreenDormindo() {
  int xEsq = 32, xDir = 96, yOlho = 26;
  display.drawFastHLine(xEsq - 14, yOlho + 5, 28, SSD1306_WHITE); 
  display.drawFastHLine(xDir - 14, yOlho + 5, 28, SSD1306_WHITE); 
  int animZ = (millis() / 1000) % 3;
  if (animZ >= 0) { display.setCursor(105, 5); display.print("z"); }
  if (animZ >= 1) { display.setCursor(115, 0); display.print("Z"); }
  if (animZ >= 2) { display.setCursor(120, -5); display.print("Z"); }
  display.setCursor(20, 56); display.print("ARGUS Standby...");
}

void drawScreenAvatar() {
  int xEsq = 32, xDir = 96, yOlho = 26, raio = 18;
  if (barulhoDetectado) { 
    display.fillCircle(xEsq, yOlho, 10, SSD1306_WHITE); 
    display.fillCircle(xDir, yOlho, 10, SSD1306_WHITE);
    display.fillCircle(xEsq, yOlho, 3, SSD1306_BLACK); 
    display.fillCircle(xDir, yOlho, 3, SSD1306_BLACK);
    display.fillCircle(64, 48, 6, SSD1306_WHITE); 
  }
  else if (valorGasSuavizado >= 1200) { 
    display.drawLine(xEsq - 10, yOlho - 10, xEsq + 10, yOlho + 10, SSD1306_WHITE);
    display.drawLine(xEsq - 10, yOlho + 10, xEsq + 10, yOlho - 10, SSD1306_WHITE);
    display.drawLine(xDir - 10, yOlho - 10, xDir + 10, yOlho + 10, SSD1306_WHITE);
    display.drawLine(xDir - 10, yOlho + 10, xDir + 10, yOlho - 10, SSD1306_WHITE);
    display.drawLine(55, 48, 73, 48, SSD1306_WHITE);
  } else { 
    if(!estaPiscando) {
      display.fillCircle(xEsq, yOlho, raio, SSD1306_WHITE);
      display.fillCircle(xDir, yOlho, raio, SSD1306_WHITE);
      int offsetPupilaX = (millis() / 3000) % 3 - 1; 
      display.fillCircle(xEsq + (offsetPupilaX * 4), yOlho, 5, SSD1306_BLACK); 
      display.fillCircle(xDir + (offsetPupilaX * 4), yOlho, 5, SSD1306_BLACK);
      display.drawLine(58, 45, 64, 49, SSD1306_WHITE);
      display.drawLine(64, 49, 70, 45, SSD1306_WHITE);
    } else {
      display.drawFastHLine(xEsq - 14, yOlho, 28, SSD1306_WHITE);
      display.drawFastHLine(xDir - 14, yOlho, 28, SSD1306_WHITE);
    }
  }
  int tamanhoTexto = falaAtual.length() * 6; 
  int xTexto = (128 - tamanhoTexto) / 2;
  display.setCursor(xTexto > 0 ? xTexto : 0, 56); display.println(falaAtual);
}

void lerSensores() {
  float temp_lida = dht.readTemperature(); float umid_lida = dht.readHumidity();
  if (!isnan(temp_lida)) { 
    temperatura = temp_lida; umidade = umid_lida; 
    indiceCalor = dht.computeHeatIndex(temperatura, umidade, false); 
  }
  int rawGas = analogRead(GAS_SENSOR_PIN);
  if (valorGasSuavizado == 0) valorGasSuavizado = rawGas; 
  else valorGasSuavizado = (FILTRO_ALFA * rawGas) + ((1.0 - FILTRO_ALFA) * valorGasSuavizado);
}

void gerarFalaDoAvatar() {
  int sorteio = random(0, 3); 
  if (vaiChover && valorGasSuavizado > 400) falaAtual = "Cheiro de chuva!";
  else if (valorGasSuavizado >= 1200) falaAtual = "Cof! Ar poluido!";
  else if (indiceCalor >= 31.0) falaAtual = "Que calor...";
  else if (temperatura < 18.0 && temperatura > 0) falaAtual = "Brrr.. frio!";
  else falaAtual = "ARGUS Monitorando";
}

void enviarDadosBluetooth() {
  if (SerialBT.hasClient()) {
    SerialBT.println("\n=== STATUS ARGUS V7.1 ===");
    SerialBT.print("Temp: "); SerialBT.print(temperatura, 1); SerialBT.println(" C");
    SerialBT.print("Gas:  "); SerialBT.println(valorGasSuavizado);
    SerialBT.println("============================");
  }
}

void enviarThingSpeak() {
  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + writeAPIKey + "&field1=" + String(temperatura) + "&field2=" + String(umidade) + "&field3=" + String(valorGasSuavizado);
  http.begin(url); http.GET(); http.end();
}

void buscarPrevisaoTempoGratuita() {
  if(WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  http.begin("http://api.open-meteo.com/v1/forecast?latitude=" + latitude + "&longitude=" + longitude + "&current_weather=true");
  if (http.GET() == 200) {
    DynamicJsonDocument doc(1024); deserializeJson(doc, http.getString());
    tempExterna = doc["current_weather"]["temperature"];
    vaiChover = (doc["current_weather"]["weathercode"] >= 51);
  }
  http.end();
}

void drawScreenSensores() {
  display.setCursor(0, 0); display.println("Sensores Locais");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  display.setCursor(0, 18); display.print("Temp: "); display.print(temperatura, 1); display.println(" C");
  display.setCursor(0, 30); display.print("Umid: "); display.print((int)umidade); display.println(" %");
  display.setCursor(0, 42); display.print("Sens: "); display.print(indiceCalor, 1); display.println(" C");
}

void drawScreenClima() {
  display.setCursor(0, 0); display.print("Satelite Externo");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  if(tempExterna == 0.0) display.setCursor(0, 25); else {
    display.setCursor(0, 18); display.print("Temp Ext: "); display.print(tempExterna, 1); display.println(" C");
  }
  if(vaiChover) { display.setCursor(0, 46); display.print("Previsao: Vai Chover!"); } 
  else { display.setCursor(0, 46); display.print("Previsao: Sem chuva."); }
}

// ================= DASHBOARD WEB (INTERFACE PROFISSIONAL COM FOTO) =================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ARGUS Dashboard</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #0b0c10; color: #c5c6c7; margin: 0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: auto; }";
  html += ".header { text-align: center; border-bottom: 2px solid #45a29e; padding-bottom: 10px; margin-bottom: 20px; }";
  html += "h1 { color: #66fcf1; margin: 0; font-size: 28px; letter-spacing: 2px; }";
  html += ".card { background: #1f2833; border-radius: 8px; padding: 20px; margin-bottom: 20px; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }";
  html += "h2 { color: #45a29e; border-bottom: 1px solid #45a29e; padding-bottom: 5px; margin-top: 0; }";
  html += "p { margin: 10px 0; font-size: 16px; }";
  html += "span.dado { color: #ffffff; font-weight: bold; float: right; }";
  
  /* Estilo do Cartão Médico com FOTO */
  html += ".medical-card { background: #4a0e0e; border-left: 5px solid #ff4a4a; padding: 20px; border-radius: 8px; display: flex; align-items: center; gap: 20px; box-shadow: 0 4px 15px rgba(255,0,0,0.2); }";
  html += ".photo-container { flex-shrink: 0; width: 120px; height: 150px; border: 3px solid #ff4a4a; border-radius: 8px; overflow: hidden; background: #000; }";
  html += ".photo-container img { width: 100%; height: 100%; object-fit: cover; }";
  html += ".info-container { flex-grow: 1; color: #ffdddd; }";
  html += ".info-container p { margin: 5px 0; font-size: 14px; border-bottom: 1px dashed rgba(255,255,255,0.2); padding-bottom: 3px;}";
  html += ".info-container span.dado { float: none; padding-left: 5px; color: #ffffff; }";
  
  /* Ajuste para telas de celular pequenas */
  html += "@media (max-width: 480px) { .medical-card { flex-direction: column; text-align: center; } .photo-container { width: 100px; height: 125px; } }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<div class='header'><h1>[ PROJETO ARGUS ]</h1><p style='color:#45a29e; font-style:italic;'>Monitoramento Ambiental e Saude em Tempo Real</p></div>";
  
  html += "<div class='card'><h2>DADOS AMBIENTAIS</h2>";
  html += "<p>Temperatura Interna: <span class='dado'>" + String(temperatura, 1) + " &deg;C</span></p>";
  html += "<p>Umidade Relativa: <span class='dado'>" + String(umidade, 1) + " %</span></p>";
  html += "<p>Qualidade do Ar (Gas): <span class='dado'>" + String(valorGasSuavizado) + "</span></p>";
  html += "<p>Alarme de Presenca: <span class='dado' style='color:" + String(avatarDormindo ? "#45a29e" : "#ff4a4a") + "'>" + String(avatarDormindo ? "SEGURO" : "MOVIMENTO!") + "</span></p>";
  html += "</div>";

  // MÓDULO MÉDICO (Aparece com a FOTO)
  if(rfidNomeCompleto != "") {
    html += "<h2 style='color:#ff4a4a; text-align:center;'>⚠ ALERTA MEDICO ACIONADO ⚠</h2>";
    html += "<div class='medical-card'>";
    
    // =========================================================================
    // COLE O LINK DA SUA FOTO AQUI DENTRO DAS ASPAS SIMPLES do src='...'
    // =========================================================================
    html += "<div class='photo-container'><img src='https://img.freepik.com/fotos-premium/um-homem-idoso-feliz-de-60-anos-de-idade-com-rosto-asiatico-olhando-diretamente-para-a-camera-com-fundo-de-cor-solida_521449-212.jpg' alt='Foto do Paciente'></div>";
    
    html += "<div class='info-container'>";
    html += "<p><b>NOME:</b> <span class='dado'>" + rfidNomeCompleto + "</span></p>";
    html += "<p><b>NASC:</b> <span class='dado'>" + rfidNasc + "</span></p>";
    html += "<p><b>TEL:</b> <span class='dado'>" + rfidTel + "</span></p>";
    html += "<p><b>SANGUE:</b> <span class='dado'>" + rfidSangue + "</span></p>";
    html += "<p><b>PCD:</b> <span class='dado'>" + rfidPCD + "</span></p>";
    html += "<p><b>CONDICAO:</b> <span class='dado'>" + rfidCondicao + "</span></p>";
    html += "<p><b>EMERG:</b> <span class='dado'>" + rfidEmergencia + "</span></p>";
    html += "</div></div>";
  }

  html += "</div></body></html>";
  server.send(200, "text/html", html);
}
