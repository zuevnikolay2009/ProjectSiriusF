#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <Servo.h>

// =============== ПИНЫ ===============
#define PWMA 11   // Левый мотор
#define AIN1 6
#define AIN2 7

#define PWMB 3    // Правый мотор
#define BIN1 5
#define BIN2 4

// Ультразвук спереди — для цилиндров
#define TRIG_FRONT 12
#define ECHO_FRONT 13

// Ультразвук вниз — для пропастей
#define TRIG_DOWN 8
#define ECHO_DOWN 9

#define SERVO_PIN 10  // серво на пине 10

int motorSpeed = 50;
int escapeSpeed = 40;

// =============== ПОРОГИ ===============
const int VOID_DISTANCE = 9; // см
const int OBJECT_DIST = 6;   // см

// =============== ГЛОБАЛЬНЫЕ ===============
String carriedColor = "NONE";
const int TOTAL_OBJECTS = 8;
int objectsDelivered = 0;
bool isFinished = false;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_60X);
Servo myServo;

void setup() {
  Serial.begin(9600);

  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  pinMode(TRIG_FRONT, OUTPUT);
  pinMode(ECHO_FRONT, INPUT);
  pinMode(TRIG_DOWN, OUTPUT);
  pinMode(ECHO_DOWN, INPUT);

  myServo.attach(SERVO_PIN);
  myServo.write(15); // открыто

  if (!tcs.begin()) while (1);

}

int getDistanceFront() {
  digitalWrite(TRIG_FRONT, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_FRONT, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_FRONT, LOW);
  long duration = pulseIn(ECHO_FRONT, HIGH, 25000);
  return (duration == 0) ? 999 : duration * 0.034 / 2;
}

int getDistanceDown() {
  digitalWrite(TRIG_DOWN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_DOWN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_DOWN, LOW);
  long duration = pulseIn(ECHO_DOWN, HIGH, 30000);
  return (duration == 0) ? 999 : duration * 0.034 / 2;
}

String detectColor() {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  float total = r + g + b;
  if (total == 0) return "WHITE";

  float r_norm = r / total;
  float g_norm = g / total;
  float b_norm = b / total;

  if (r_norm > 0.36) return "RED";
  if (g_norm > 0.35) return "GREEN";
  if (b_norm > 0.44) return "BLUE";
  if (c < 12000) return "BLACK";

  return "WHITE";
}

void forward(int spd) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMA, spd);
  analogWrite(PWMB, spd);
}

void backward(int spd) {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, spd);
  analogWrite(PWMB, spd);
}

void turnRight() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, escapeSpeed);
  analogWrite(PWMB, escapeSpeed);
}

void stopMotors() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
  int distFront = getDistanceFront();
  int distDown = getDistanceDown();
  String color = detectColor();

  Serial.println(distFront);
  Serial.println(distDown);
  Serial.println(color);

    // 🔴 АВАРИЯ: пропасть под роботом
if (distDown > VOID_DISTANCE) {
// Если держим груз — немедленно сбросить!
if (carriedColor == "NONE") {
  stopMotors();
  delay(200);
  backward(motorSpeed / 2); delay(1000);
  stopMotors(); delay(200);
  turnRight(); delay(600);
  stopMotors();

    
  }
}

 if (carriedColor != "NONE") {
    forward(motorSpeed);
    Serial.println(color);
    // Любой люк — сбрасываем
    if (color == "BLACK" || distDown > 9) {
      delay(200);
      stopMotors();
      myServo.write(15); delay(1000);
      backward(motorSpeed / 2); delay(500);
      stopMotors();
      carriedColor = "NONE";
      objectsDelivered++;
      if (objectsDelivered >= TOTAL_OBJECTS) isFinished = true;
    }
    return;
  }

forward(motorSpeed);

  // Избегаем люков без груза
  if (color == "RED" || color == "GREEN" || color == "BLUE" || color == "BLACK") {