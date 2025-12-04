#pragma once
#include <Arduino.h>

// Pinos da ponte H (ajusta se necessário)
const int PIN_MOTOR_IN1 = 25;
const int PIN_MOTOR_IN2 = 26;
const int PIN_MOTOR_IN3 = 27;
const int PIN_MOTOR_IN4 = 14;

// Pinos PWM (ENA / ENB do driver de motor)
const int PIN_ENA = 32;   // motor A (IN1, IN2)
const int PIN_ENB = 33;   // motor B (IN3, IN4)

// Velocidade global (0–255)
int motorSpeed = 80;      // baixa velocidade para testes

void setupMotores() {
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);

  // Começa parado
  analogWrite(PIN_ENA, 0);
  analogWrite(PIN_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
}

void parar() {
  analogWrite(PIN_ENA, 0);
  analogWrite(PIN_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
}

void andarFrente() {
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);

  analogWrite(PIN_ENA, motorSpeed);
  analogWrite(PIN_ENB, motorSpeed);
}

void andarTras() {
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, HIGH);

  analogWrite(PIN_ENA, motorSpeed);
  analogWrite(PIN_ENB, motorSpeed);
}

void rodarEsquerda() {
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);

  analogWrite(PIN_ENA, motorSpeed);
  analogWrite(PIN_ENB, motorSpeed);
}

void rodarDireita() {
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, HIGH);

  analogWrite(PIN_ENA, motorSpeed);
  analogWrite(PIN_ENB, motorSpeed);
}
