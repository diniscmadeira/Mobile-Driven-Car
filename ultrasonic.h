----------ULTRASONIC.H----------      #include <HardwareSerial.h>

// UART2 no ESP32
HardwareSerial SensorSerial(2);

#define RXD2 16   // TX do sensor -> GPIO16 (RX)
#define TXD2 17   // não usado para o sensor

void setupUltrasonic() {
  SensorSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
}

float getDistance() {
  unsigned char data[4] = {};
  float distance = -1;

  // Se houver muito lixo no buffer, limpa até sobrar só um pacote
  while (SensorSerial.available() > 4) {
    SensorSerial.read();
  }

  if (SensorSerial.available() < 4) {
    return -1;
  }

  for (int i = 0; i < 4; i++) {
    data[i] = SensorSerial.read();
  }

  if (data[0] == 0xFF) {
    int sum = (data[0] + data[1] + data[2]) & 0x00FF;
    if (sum == data[3]) {
      distance = (data[1] << 8) + data[2];
      return (distance/10);   // bruto (cm ou mm, conforme o teu módulo)
    }
  }
  return -1;
}
