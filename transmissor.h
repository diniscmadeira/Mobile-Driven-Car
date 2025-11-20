#include <WiFi.h>
#include <esp_now.h>

uint8_t peerMac[] = { 0x08, 0xB6, 0x1F, 0xEF, 0x8A, 0x94 };

void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // callback vazio para não imprimir "Send status"
}

void setupTransmissor() {
  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void enviarMensagemEspNow(const char* msg) {
  esp_now_send(peerMac, (uint8_t *)msg, strlen(msg));
}
