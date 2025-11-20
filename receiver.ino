#include <WiFi.h>
#include <esp_now.h>

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  char valorRecebido[len + 1];
  memcpy(valorRecebido, incomingData, len);
  valorRecebido[len] = '\0'; // Garante string válida
  Serial.println(valorRecebido);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Vazio – recebe por callback
}
