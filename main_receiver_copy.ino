#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include "ultrasonic.h"
#include "motores.h"
#include "Servo.h"

WebServer server(80);

//-------------------------------------------------LED----------------------------------

// pinos do LED RGB
const int PIN_LED_R = 13;
const int PIN_LED_G = 12;
const int PIN_LED_B = 22;

void setColor(bool r, bool g, bool b) {
  digitalWrite(PIN_LED_R, r ? HIGH : LOW);
  digitalWrite(PIN_LED_G, g ? HIGH : LOW);
  digitalWrite(PIN_LED_B, b ? HIGH : LOW);
}

//---------------------------button-------------------------
#define BUTTON_PIN 2

int buttonState;
int lastButtonState = HIGH;              // the previous reading
unsigned long lastDebounceTime = 0;      // the last time
const unsigned long debounceDelay = 50;  // the

bool buttonPressed() {
  int reading = digitalRead(BUTTON_PIN);
  bool pressed = false;
  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == LOW) {
        pressed = true;
      }
    }
  }
  lastButtonState = reading;
  return pressed;
}

// ---------------------- MODOS DO CARRO -----------------------------------

enum Modo {
  MODO_GIRO,  // controlado pela luva (giroscópio -> F,B,L,R,S)
  MODO_APP,   // controlado pela app do telemóvel (a implementar)
  MODO_AUTO   // condução autónoma com ultrassónico
};

Modo modoAtual = MODO_GIRO;

// ---------------------- TEMPORIZACAO ULTRASOM -----------------------------------
unsigned long ultimoRead = 0;
const unsigned long intervalo = 35;

// ---------------------- MODO GIROSCÓPIO -----------------------------

enum EstadoGiro {
  Frente,
  Tras,
  Esquerda,
  Direita,
  Parar
};

EstadoGiro estadoGiro = Parar;

void loopModoGiro() {
  switch (estadoGiro) {
    case Frente:
      andarFrente();
      break;

    case Tras:
      andarTras();
      break;

    case Esquerda:
      rodarEsquerda();
      break;

    case Direita:
      rodarDireita();
      break;

    case Parar:
    default:
      parar();
      break;
  }
}

// ---------------------- RECEIVER -----------------------------------

const int TAM_MAX = 32;  // tamanho máximo da mensagem
char sentido[TAM_MAX];   // buffer global para a string recebida
int tamanhoSentido = 0;
bool novoSentido = false;  //flag de comando novo

void OnDataRecv(const esp_now_recv_info_t* info, const uint8_t* incomingData, int len) {
  // limita ao tamanho do buffer

  if (modoAtual == MODO_APP) {
    return;
  }

  if (len >= TAM_MAX) len = TAM_MAX - 1;

  memcpy(sentido, incomingData, len);  // copia todos os bytes recebidos
  sentido[len] = '\0';                 // garante string terminada
  tamanhoSentido = len;
  novoSentido = true;  // marca que há dados novos

  char cmd = sentido[0];
  switch (cmd) {
    case 'F': estadoGiro = Frente; break;
    case 'B': estadoGiro = Tras; break;
    case 'L': estadoGiro = Esquerda; break;
    case 'R': estadoGiro = Direita; break;
    case 'S':
    default: estadoGiro = Parar; break;
  }

  Serial.print("Recebido: ");
  Serial.println(cmd);
  Serial.print("estadoGiro: ");
  Serial.println(estadoGiro);
}
// ---------------------- MODO APP -----------------------------

void loopModoApp() {
  // No modo APP, os comandos vêm do telemóvel (HTTP /cmd),
  // mas quem manda mexer os motores continua a ser o estadoGiro.
  loopModoGiro();
}

// ---------------------- MODO AUTÓNOMO -----------------------------

//filtro de leituras do sensor
long ultimaDistValida = -1;

long getDistanceFiltrada() {
  long d = getDistance();  // vem do ultrasonic.h

  // rejeitar leituras inválidas ou absurdas
  if (d <= 0 || d > 300) {  // ajusta 300 ao alcance do teu sensor
    return -1;
  }

  ultimaDistValida = d;
  return ultimaDistValida;
}


extern int motorSpeed;
int motorSpeedBackup = 0;

enum EstadoAuto {
  AUTO_ANDAR_FRENTE,
  AUTO_PARAR_ANALISAR,
  AUTO_OLHAR_ESQ,
  AUTO_OLHAR_DIR,
  AUTO_RECUAR,
  AUTO_VIRAR
};

EstadoAuto estadoAuto = AUTO_ANDAR_FRENTE;

unsigned long autoPrevMillis = 0;
const unsigned long autoTempoParar = 200;
const unsigned long autoTempoScan = 500;  // tempo para o servo chegar à posição
const unsigned long autoTempoRecuar = 400;
const unsigned long autoTempoVirar = 900;

//desencravar
long distFrentePrev = -1;
unsigned long stuckStartMillis = 0;
const int TOL_DIST = 1;              // tolerância em cm
const unsigned long T_STUCK = 1200;  // tempo para considerar encravado

const int DIST_SEGURA_CM = 25;

long distFrente = -1;
long distEsq = -1;
long distDir = -1;


void loopModoAuto() {
  unsigned long agora = millis();

  switch (estadoAuto) {

    case AUTO_ANDAR_FRENTE:
      {
        if (agora - ultimoRead >= intervalo) {
          ultimoRead = agora;
          distFrente = getDistanceFiltrada();  // usa filtro

          if (distFrente == -1) {
            // não há leitura válida, não atualizes lógica de stuck
            break;  // sai do case, na próxima volta tenta de novo
          }
          Serial.println(distFrente);

          // detetar obstáculo "normal"
          if (distFrente > 0 && distFrente <= DIST_SEGURA_CM) {
            parar();
            autoPrevMillis = agora;
            estadoAuto = AUTO_PARAR_ANALISAR;
            distFrentePrev = -1;   // reset
            stuckStartMillis = 0;  // reset
            break;
          }

          // detetar possível encravamento (distância quase igual)
          if (distFrente > 0 && distFrentePrev > 0 && abs(distFrente - distFrentePrev) <= TOL_DIST) {

            if (stuckStartMillis == 0) {
              stuckStartMillis = agora;  // começou situação estável
            } else if (agora - stuckStartMillis >= T_STUCK) {
              // considerado encravado -> manobra direta
              parar();
              autoPrevMillis = agora;
              estadoAuto = AUTO_PARAR_ANALISAR;
              // opcional: limpar para próxima vez
              stuckStartMillis = 0;
              distFrentePrev = -1;
              break;
            }
          } else {
            // distância mudou significativamente, não está "preso"
            stuckStartMillis = 0;
          }

          distFrentePrev = distFrente;

          // se não há obstáculo e não está preso, segue em frente
          if (estadoAuto == AUTO_ANDAR_FRENTE) {
            andarFrente();
          }
        }
        break;
      }
    case AUTO_PARAR_ANALISAR:
      {
        if (agora - autoPrevMillis >= autoTempoParar) {
          olharEsquerda();  // Servo.h
          autoPrevMillis = agora;
          estadoAuto = AUTO_OLHAR_ESQ;
        }
        break;
      }

    case AUTO_OLHAR_ESQ:
      {
        if (agora - autoPrevMillis >= autoTempoScan) {
          distEsq = getDistanceFiltrada();
          olharDireita();
          autoPrevMillis = agora;
          estadoAuto = AUTO_OLHAR_DIR;
        }
        break;
      }

    case AUTO_OLHAR_DIR:
      {
        if (agora - autoPrevMillis >= autoTempoScan) {
          distDir = getDistanceFiltrada();
          olharFrente();
          autoPrevMillis = agora;

          // recuar antes de virar
          andarTras();
          estadoAuto = AUTO_RECUAR;
        }
        break;
      }

    case AUTO_RECUAR:
      {
        if (agora - autoPrevMillis >= autoTempoRecuar) {
          parar();
          autoPrevMillis = agora;

          // escolhe o lado com mais espaço
          motorSpeedBackup = motorSpeed;
          motorSpeed = 150;  // mais forte só para rodar no lugar

          if (distEsq > distDir) {
            rodarEsquerda();
          } else {
            rodarDireita();
          }

          estadoAuto = AUTO_VIRAR;
        }
        break;
      }

    case AUTO_VIRAR:
      {
        if (agora - autoPrevMillis >= autoTempoVirar) {  // ex.: 900 ms
          parar();
          motorSpeed = motorSpeedBackup;  // volta à velocidade normal
          ultimoRead = agora;
          estadoAuto = AUTO_ANDAR_FRENTE;
        }
        break;
      }
  }
}


void setup() {
  Serial.begin(9600);

  Serial.println("boot");

  //Servo:
  setupServo();

  // Ultrassónic
  setupUltrasonic();

  //button
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(BUTTON_PIN);

  //Led RGB
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setColor(false, true, false);

  // Motores (pinos reservados em motores.h)
  setupMotores();
  motorSpeed = 120;

  // --- Wi‑Fi AP para o telemóvel + ESP‑NOW para a luva ---
  const char* apSsid = "CarroESP";
  const char* apPassword = "12345678";

  WiFi.mode(WIFI_AP_STA);  // AP + STA, compatível com ESP‑NOW [web:36][web:199]
  bool apOk = WiFi.softAP(apSsid, apPassword);
  if (!apOk) {
    Serial.println("Falha ao criar AP");
  }
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());  // tipicamente 192.168.4.1 [web:36]

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao iniciar ESP-NOW");
  } else {
    esp_now_register_recv_cb(OnDataRecv);  // o mesmo callback que já tens
    if (esp_now_init() != ESP_OK) {
      Serial.println("Erro ao iniciar ESP‑NOW");
    } else {
      esp_now_register_recv_cb(OnDataRecv);

      // --------- Servidor HTTP ----------

      // "/" -> usado pelo botão Conectar da app
      server.on("/", HTTP_GET, []() {
        server.send(200, "text/plain", "OK");
      });

      // "/cmd?c=X" -> comandos F/T/E/D/S da app
      server.on("/cmd", HTTP_GET, []() {
        if (!server.hasArg("c")) {
          server.send(400, "text/plain", "Missing c");
          return;
        }

        char cmd = server.arg("c")[0];

        switch (cmd) {
          case 'F': estadoGiro = Frente; break;
          case 'T': estadoGiro = Tras; break;
          case 'E': estadoGiro = Esquerda; break;
          case 'D': estadoGiro = Direita; break;
          case 'S':
          default: estadoGiro = Parar; break;
        }

        String reply = String("cmd=") + cmd;
        server.send(200, "text/plain", reply);
        Serial.println(reply);
      });

      server.begin();
      Serial.println("HTTP server iniciado");
    }
  }

  // Por padrão, começa controlado pela luva
  modoAtual = MODO_GIRO;
}

void loop() {
  server.handleClient();
  switch (modoAtual) {
    case MODO_GIRO //azul
      loopModoGiro();
      if (buttonPressed()) {
        setColor(false, false, true);  //azul
        modoAtual = MODO_APP;
      }
      break;

    case MODO_APP: //verde
      loopModoApp();
      if (buttonPressed()) {
        setColor(false, true, false);  
        modoAtual = MODO_AUTO;
      }
      break;

    case MODO_AUTO: //vermelho
      loopModoAuto();
      if (buttonPressed()) {
        setColor(true, false, false); 
        modoAtual = MODO_GIRO;
      }
      break;

    default: modoAtual = MODO_GIRO; break;
  }
}
