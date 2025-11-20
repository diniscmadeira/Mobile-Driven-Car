#include "transmissor.h"
#include <Wire.h>

const int MPU = 0x68;
int16_t AcX, AcY, AcZ;
char ultimoSentido = 0;

void setup() {
  Serial.begin(115200);
  setupTransmissor();
  Wire.begin(21, 22);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop() {
  char sentido = 'S';

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  float anguloX = atan2(AcY, sqrt(AcX * AcX + AcZ * AcZ)) * 180 / PI;
  float anguloY = atan2(AcX, sqrt(AcY * AcY + AcZ * AcZ)) * 180 / PI;

  if (anguloX < -35 && anguloY > -35 && anguloY < 35){
    sentido = 'B';
  } else if (anguloX > 35 && anguloY > -35 && anguloY < 35){
    sentido = 'F';
  } else if (anguloY < -35 && anguloX > -35 && anguloX < 35){
    sentido = 'L';
  } else if (anguloY > 35 && anguloX > -35 && anguloX < 35){
    sentido = 'R';
  } else if (anguloX > -35 && anguloY > -35 && anguloX < 35 && anguloY < 35){
    sentido = 'S';
  }

  char buffer[2];
  buffer[0] = sentido;
  buffer[1] = '\0';
  enviarMensagemEspNow(buffer);
  Serial.println(sentido);

  delay(100);
}

