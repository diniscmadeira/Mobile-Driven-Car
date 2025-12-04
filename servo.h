#pragma once

#include <ESP32Servo.h>

// Pino do servo (ajusta ao teu hardware)
const int PIN_SERVO     = 18;
const int SERVO_MIN_US  = 500;   // range típico SG90 [web:1][web:88]
const int SERVO_MAX_US  = 2400;

Servo sg90;

void setupServo() {
  sg90.setPeriodHertz(50);  // 50 Hz para servo [web:1][web:85]
  sg90.attach(PIN_SERVO, SERVO_MIN_US, SERVO_MAX_US);
  sg90.write(90);           // posição “frente”
}

void olharFrente() {
  sg90.write(90);
}

void olharDireita() {
  sg90.write(30);           // ajusta conforme o teu robô [web:7][web:89]
}

void olharEsquerda() {
  sg90.write(150);
}
