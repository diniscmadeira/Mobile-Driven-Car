----------MAIN_RECEIVER_COPY---------- #include <WiFi.h>
#include <esp_now.h>
#include "ultrasonic.h"
#include "motores.h"

// ---------------------- MODOS DO CARRO -----------------------------------

enum Modo {
  MODO_GIRO,  // controlado pela luva (giroscópio -> F,B,L,R,S)
  MODO_APP,   // controlado pela app do telemóvel (a implementar)
  MODO_AUTO   // condução autónoma com ultrassónico
};

Modo modoAtual = MODO_GIRO;

// ---------------------- TEMPORIZACAO ULTRASOM -----------------------------------
unsigned long ultimoRead = 0;
const unsigned long intervalo = 30;

//usado nos valores lidos da luva
char ultimoSentido = 0;

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
  if (!novoSentido) return;

  char cmd = sentido[0];   // 'F','B','L','R','S', ...

  Serial.print("[GIRO] Comando: ");
  Serial.println(cmd);

  switch (cmd) {
    case 'F': andarFrente();   break;
    case 'B': andarTras();     break;
    case 'L': rodarEsquerda(); break;
    case 'R': rodarDireita();  break;
    default:  parar();         break;
  }

  novoSentido = false;
}*/

// ---------------------- MODO APP -----------------------------

/*void loopModoApp() {
  // FUTURO:
  // - ler comandos da app (WiFi/Bluetooth)
  // - mapear para F,B,L,R,S
  // - chamar as funções de movimento como em loopModoGiro()
}/*

// ---------------------- MODO AUTÓNOMO -----------------------------

/*void loopModoAuto() {
  unsigned long agora = millis();
  if (agora - ultimoRead < intervalo) return;
  ultimoRead = agora;

  float cm = getDistance();
  if (cm <= 25) {
    parar();
    return;
  }

  // Ajusta: se o teu A02YYUW for mm -> /10; se já for cm -> (int)cm
  int distanciaCm = cm / 10;   // ou: int distanciaCm = (int)cm;

  const int LIMIAR_OBST = 25;   // cm

  if (distanciaCm > LIMIAR_OBST) {
    andarFrente();
  } else {
    parar();
    // FUTURO:
    // - girar um pouco,
    // - ou fazer scan de 180° com servo e escolher melhor direção
  }
}*/


void setup() {
  Serial.begin(9600);

  // Ultrassónic
  setupUltrasonic();

  // Motores (pinos reservados em motores.h)
  setupMotores();

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);

  // Por padrão, começa controlado pela luva
  modoAtual = MODO_GIRO;
}

void loop() {
  // Exemplo simples de mudança de modo temporária:
  // (mais tarde isto será feito por botão físico ou comando da app)
  //
  // if (Serial.available()) {
  //   char c = Serial.read();
  //   if      (c == '1') modoAtual = MODO_GIRO;
  //   else if (c == '2') modoAtual = MODO_APP;
  //   else if (c == '3') modoAtual = MODO_AUTO;
  // }

  /*switch (modoAtual) {
    case MODO_GIRO:
      loopModoGiro();
      break;

    case MODO_APP:
      loopModoApp();
      break;

    case MODO_AUTO:
      loopModoAuto();
      break;
  }*/

  float d = getDistance();
  if (d > 0) {
    int distanciaCm = d;  // ou (int)d se já for cm
    Serial.print("Distancia: ");
    Serial.println(distanciaCm);
  }
  delay(100);

}
