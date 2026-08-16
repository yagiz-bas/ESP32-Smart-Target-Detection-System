#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =========================
// PINLER
// =========================

const int SERVO_PIN = 18;

const int TRIG_PIN = 5;
const int ECHO_PIN = 4;

const int BUZZER_PIN = 25;
const int LED_PIN = 26;
const float LOST_DISTANCE = 45.0;
const int LOST_LIMIT = 3;

int lostCount = 0;  
// =========================
// OLED
// =========================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =========================
// SERVO
// =========================

Servo radarServo;

const int MIN_ANGLE = 30;
const int MAX_ANGLE = 150;

int currentAngle = 90;
int scanDirection = 1;

// =========================
// HEDEF
// =========================

const float DETECT_DISTANCE = 35.0;

bool locked = false;
int detectCount = 0;

float lockedDistance = 0;


// =========================
// MESAFE ÖLÇ
// =========================

float readDistance() {

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(
    ECHO_PIN,
    HIGH,
    25000
  );

  if (duration == 0) {
    return 400.0;
  }

  float distance =
    duration * 0.0343 / 2.0;

  return distance;
}


// =========================
// OLED SCANNING
// =========================

void showScanning(float distance) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(10, 0);
  display.println("SCANNING");

  display.setTextSize(1);

  display.setCursor(0, 30);
  display.print("Distance: ");
  display.print(distance, 1);
  display.println(" cm");

  display.setCursor(0, 45);
  display.print("Angle: ");
  display.print(currentAngle);
  display.println(" deg");

  display.display();
}


// =========================
// OLED LOCKED
// =========================

void showLocked(float distance) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(8, 0);
  display.println("TARGET");

  display.setCursor(15, 20);
  display.println("LOCKED");

  display.setTextSize(1);

  display.setCursor(0, 43);
  display.print("DIST: ");
  display.print(distance, 1);
  display.println(" cm");

  display.setCursor(0, 55);
  display.print("ANGLE: ");
  display.print(currentAngle);
  display.println(" deg");

  display.display();
}


// =========================
// SERVO HAREKET
// =========================

void moveServo(int angle) {

  currentAngle = constrain(
    angle,
    MIN_ANGLE,
    MAX_ANGLE
  );

  radarServo.write(currentAngle);

  delay(100);
}


// =========================
// ALARM
// =========================

void alarmOn() {

  digitalWrite(LED_PIN, HIGH);

  // Buzzer
  tone(BUZZER_PIN, 2000);
}


void alarmOff() {

  digitalWrite(LED_PIN, LOW);

  noTone(BUZZER_PIN);
}


// =========================
// SETUP
// =========================

void setup() {

  Serial.begin(115200);

  // HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // Servo
  radarServo.setPeriodHertz(50);

  radarServo.attach(
    SERVO_PIN,
    500,
    2400
  );

  currentAngle = 90;
  radarServo.write(currentAngle);

  // OLED
  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDR
      )) {

    Serial.println("OLED BULUNAMADI!");

    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);

  display.setCursor(15, 10);
  display.println("RADAR");

  display.setCursor(20, 35);
  display.println("SYSTEM");

  display.display();

  delay(1500);

  Serial.println();
  Serial.println("==========================");
  Serial.println(" ESP32 RADAR SYSTEM");
  Serial.println("==========================");
  Serial.println();
}


// =========================
// LOOP
// =========================

void loop() {

  // ==================================================
  // KİLİTLİ DURUM
  // ==================================================

  if (locked) {

  float distance = readDistance();

  lockedDistance = distance;

  showLocked(distance);

  alarmOn();

  Serial.print("TARGET LOCKED | ");
  Serial.print("Angle: ");
  Serial.print(currentAngle);
  Serial.print(" | Distance: ");
  Serial.print(distance, 1);
  Serial.println(" cm");

  // Hedef hâlâ yakın
  if (distance <= LOST_DISTANCE) {

    lostCount = 0;
  }

  // Hedef kaybolmaya başladı
  else {

    lostCount++;

    Serial.print("TARGET LOST CHECK: ");
    Serial.print(lostCount);
    Serial.print("/");
    Serial.println(LOST_LIMIT);

    // Henüz tamamen kayıp değil
    if (lostCount < LOST_LIMIT) {

      delay(300);
      return;
    }

    // ------------------------------------------
    // HEDEF TAMAMEN KAYBOLDU
    // ------------------------------------------

    Serial.println();
    Serial.println("==========================");
    Serial.println("      TARGET LOST");
    Serial.println("      RESUMING SCAN");
    Serial.println("==========================");
    Serial.println();

    locked = false;
    lostCount = 0;
    detectCount = 0;

    alarmOff();

    // Mevcut açıdan taramaya devam et
    // Baştan 90 dereceye dönmesine gerek yok.

    delay(500);

    return;
  }

  delay(300);

  return;
}


  // ==================================================
  // TARAMA
  // ==================================================

  moveServo(currentAngle);

  float distance = readDistance();

  Serial.print("SCAN | Angle: ");
  Serial.print(currentAngle);

  Serial.print(" | Distance: ");
  Serial.print(distance, 1);

  Serial.println(" cm");

  showScanning(distance);


  // ==================================================
  // HEDEF ALGILANDI
  // ==================================================

  if (distance < DETECT_DISTANCE) {

    detectCount++;

    Serial.print("TARGET CHECK ");
    Serial.print(detectCount);
    Serial.println("/3");

  }
  else {

    detectCount = 0;
  }


  // ==================================================
  // 3 ARDIŞIK ÖLÇÜM
  // ==================================================

  if (detectCount >= 3) {

    locked = true;

    lockedDistance = distance;

    Serial.println();
    Serial.println("==========================");
    Serial.println("      TARGET LOCKED");
    Serial.print("      ANGLE: ");
    Serial.print(currentAngle);
    Serial.println(" deg");

    Serial.print("      DIST: ");
    Serial.print(distance, 1);
    Serial.println(" cm");

    Serial.println("==========================");
    Serial.println();

    showLocked(distance);
    alarmOn();

    return;
  }


  // ==================================================
  // TARAMA YÖNÜ
  // ==================================================

  currentAngle += scanDirection * 3;


  if (currentAngle >= MAX_ANGLE) {

    currentAngle = MAX_ANGLE;

    scanDirection = -1;
  }


  if (currentAngle <= MIN_ANGLE) {

    currentAngle = MIN_ANGLE;

    scanDirection = 1;
  }

  delay(50);
}