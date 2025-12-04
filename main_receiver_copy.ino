#include <WiFi.h>
#include <esp_now.h>
#include "ultrasonic.h"
#include "motores.h"
#include "Servo.h"

// ---------------------- MODOS DO CARRO -----------------------------------

enum Modo {
  MODO_GIRO,  // controlado pela luva (giroscópio -> F,B,L,R,S)
  MODO_APP,   // controlado pela app do telemóvel (a implementar)
  MODO_AUTO   // condução autónoma com ultrassónico
};

Modo modoAtual = MODO_GIRO;

// ---------------------- TEMPORIZACAO ULTRASOM -----------------------------------
unsigned long ultimoRead = 0;
const unsigned long intervalo = 60;

// ---------------------- RECEIVER -----------------------------------

const int TAM_MAX = 32;  // tamanho máximo da mensagem
char sentido[TAM_MAX];   // buffer global para a string recebida
int tamanhoSentido = 0;
bool novoSentido = false;  //flag de comando novo

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  // limita ao tamanho do buffer
  if (len >= TAM_MAX) len = TAM_MAX - 1;

  memcpy(sentido, incomingData, len);  // copia todos os bytes recebidos
  sentido[len] = '\0';                 // garante string terminada
  tamanhoSentido = len;
  novoSentido = true;  // marca que há dados novos
}

// ---------------------- MODO GIROSCÓPIO -----------------------------

/*void loopModoGiro() {
  
}*/

// ---------------------- MODO APP -----------------------------

/*void loopModoApp() {
  
}*/

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
const unsigned long autoTempoRecuar = 600;
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

  //Servo:
  setupServo();

  // Ultrassónic
  setupUltrasonic();

  // Motores (pinos reservados em motores.h)
  setupMotores();
  motorSpeed = 100;

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  // Por padrão, começa controlado pela luva
  modoAtual = MODO_AUTO;
}

void loop() {
  switch (modoAtual) {
    case MODO_GIRO:
      // loopModoGiro();  // por implementar
      break;

    case MODO_APP:
      // loopModoApp();   // por implementar
      break;

    case MODO_AUTO:
      loopModoAuto();
      break;
  }
}
