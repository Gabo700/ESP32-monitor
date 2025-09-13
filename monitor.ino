// Arduino json é nescessaria para intrepretar o json enviado do python, TFT_eSPI é a biblioteca que
// se usa por padrão para exibir algo na tela do ESP32
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();

// Variáveis do sistema
// Armazena os dados recebidos do python 
float cpuUsage = 0, cpuTemp = 0;
int ramPercent = 0, diskPercent = 0;
float gpuUsage = 0, gpuTemp = 0;
String timeStr = "", hostname = "", kernel = "", user = "";

// Status de conexão
// Enquanto não recebe um json valido mosta a tela de espera 
bool connected = false;

// Cores
#define TFT_GRAY 0x7BEF
#define TFT_ORANGE 0xFDA0
#define TFT_GREEN 0x07E0
#define TFT_BLUE 0x001F
#define TFT_RED 0xF800
#define TFT_CYAN 0x07FF

// Mascotes
// Skull
const char* skullFrames[] = {
  "  _____ \n /     \\ \n| (•) (•) |\n \\  ^  / \n  |||||  \n  -----  ",
  "  _____ \n /     \\ \n| (o) (o) |\n \\  ▽  / \n  |||||  \n  -----  ",
  "  _____ \n /     \\ \n| (•) (•) |\n \\  ◡  / \n  |||||  \n  -----  "
};
int skullFramesCount = sizeof(skullFrames)/sizeof(skullFrames[0]);

// Cat
const char* catFrames[] = {
  " /\\_/\\ \n( o_o ) \n > ^ < ",
  " /\\_/\\ \n( •_• ) \n > ^ < ",
  " /\\_/\\ \n( o•_• ) \n > ^ < "
};
int catFramesCount = sizeof(catFrames)/sizeof(catFrames[0]);

// Controle das animações 
int currentFrame = 0;
unsigned long lastFrameTime = 0;
const int frameDelay = 300; // ms
int xOffset = 0;

// Alternância de mascotes a cada 8 segundos
int activeMascot = 0; // 0=Caveira, 1=Cat
unsigned long lastMascotSwitch = 0;
const unsigned long mascotSwitchInterval = 8000;

// Matrix Lateral
#define MATRIX_COLS 15
#define MATRIX_ROWS 20
int matrixPosLeft[MATRIX_COLS];
int matrixPosRight[MATRIX_COLS];

void setupMatrix() {
  for (int i = 0; i < MATRIX_COLS; i++) {
    matrixPosLeft[i] = random(MATRIX_ROWS);
    matrixPosRight[i] = random(MATRIX_ROWS);
  }
}

void drawMatrixSide(int xStart, int posArray[], int yStartMatrix) {
  int matrixHeight = 90; // altura da área da Matrix igual à do mascote
  for (int i = 0; i < MATRIX_COLS; i++) {
    int y = posArray[i]*12 + yStartMatrix;
    int x = xStart + i*7;

    // Apaga anterior
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(x, y - 12);
    tft.print(char(random(33, 126)));

    // Desenha novo
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(x, y);
    tft.print(char(random(33, 126)));

    posArray[i]++;
    if (posArray[i]*12 > matrixHeight) posArray[i] = 0;
  }
}

// Setup 
// Inicia a comunicação serial no mesmo baud rate que o python, inicializa o display TFT 
// Preenche a tela de preto, configura a matrix lateral
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  setupMatrix();
}

// Loop
// Parte 1:  Lê string json enviada pelo PC faz parse com ArduinoJson atualiza as variaveis do sistema e marca
// connected = true e chama o updateDisplay

// Parte 2: Se não conectado mostra mensagem de espera com animação até receber os dadaos

// Parte 3:  Atualiza animação, atualiza o frame atual dos mascotes e move a matrix
void loop() {
  // Serial data
  if (Serial.available() > 0) {
    String jsonData = Serial.readStringUntil('\n');
    DynamicJsonDocument doc(2048);
    if (!deserializeJson(doc, jsonData)) {
      cpuUsage = doc["cpu"];
      cpuTemp = doc["cpu_temp"];
      gpuUsage = doc["gpu"]["usage"];
      gpuTemp = doc["gpu"]["temp"];
      ramPercent = doc["ram"]["percent"];
      diskPercent = doc["disk"]["percent"];
      timeStr = doc["time"].as<String>();
      hostname = doc["system"]["hostname"].as<String>();
      kernel = doc["system"]["kernel"].as<String>();
      user = doc["system"]["user"].as<String>();

      connected = true; // conexão estabelecida
      updateDisplay();
    }
  }

  // Se não conectado, mostra tela de espera
  if (!connected) {
    if (millis() - lastFrameTime > frameDelay) {
      showWaitingScreen();
      lastFrameTime = millis();
    }
    return; // não continua o resto do loop
  }

  // Troca automática de mascote
  if (millis() - lastMascotSwitch > mascotSwitchInterval) {
    activeMascot = (activeMascot + 1) % 2; // apenas 2 mascotes
    lastMascotSwitch = millis();
  }

  // Animação do mascote e matrix
  if (millis() - lastFrameTime > frameDelay) {
    currentFrame++;
    xOffset = random(-2, 3);

    int yMascotStart = 155; // abaixo das infos
    switch(activeMascot){
      case 0:
        drawFrame(skullFrames, skullFramesCount, currentFrame, xOffset, yMascotStart);
        break;
      case 1:
        drawFrame(catFrames, catFramesCount, currentFrame, xOffset, yMascotStart);
        break;
    }

    int yMatrixStart = 150; // logo abaixo das infos
    drawMatrixSide(0, matrixPosLeft, yMatrixStart);       // esquerda
    drawMatrixSide(240, matrixPosRight, yMatrixStart);    // direita

    lastFrameTime = millis();
  }
}

// Display 
void updateDisplay() {
  tft.fillScreen(TFT_BLACK);

  // Info sistema
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.printf("%s@%s", user.c_str(), hostname.c_str());

  tft.setTextSize(1);
  tft.setCursor(5, 25);
  tft.printf("Kernel: %s", kernel.c_str());
  tft.setCursor(5, 40);
  tft.printf("Time: %s", timeStr.c_str());

  tft.drawLine(0, 55, 320, 55, TFT_GRAY);

  tft.setTextSize(2);
  tft.setCursor(5, 65);
  tft.setTextColor(TFT_RED);
  tft.printf("CPU: %.1f%% %.1fC", cpuUsage, cpuTemp);

  tft.setCursor(5, 85);
  tft.setTextColor(TFT_BLUE);
  tft.printf("RAM: %d%%", ramPercent);

  tft.setCursor(5, 105);
  tft.setTextColor(TFT_ORANGE);
  tft.printf("DISK: %d%%", diskPercent);

  tft.setCursor(5, 125);
  tft.setTextColor(TFT_GREEN);
  tft.printf("GPU: %.1f%% %.1fC", gpuUsage, gpuTemp);

  tft.drawLine(0, 145, 320, 145, TFT_GRAY);
}

// Tela de espera 
void showWaitingScreen() {
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(2);
  tft.setCursor(40, 100);
  tft.println("Aguardando Conexao...");

  // Mascote animado no rodapé
  int yMascotStart = 155;
  currentFrame++;
  xOffset = random(-2,3);
  drawFrame(skullFrames, skullFramesCount, currentFrame, xOffset, yMascotStart);
}

// Desenha Mascote 
void drawFrame(const char* frames[], int totalFrames, int frameNum, int xShift, int yStart){
  int frameIndex = frameNum % totalFrames;
  String frameStr = frames[frameIndex];

  // Limpa área do mascote
  tft.fillRect(80, yStart, 160, 90, TFT_BLACK);

  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(2);

  int line = 0;
  int start = 0;
  int xStart = 120 + xShift; // centralizado horizontalmente

  for (int i = 0; i < frameStr.length(); i++) {
    if (frameStr[i] == '\n' || i == frameStr.length()-1) {
      String ln = frameStr.substring(start, i);
      tft.setCursor(xStart, yStart + line*12);
      tft.println(ln);
      start = i+1;
      line++;
    }
  }
}
