#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define LDR_PIN A0
#define LED_PIN 3
#define BUTTON_PIN 2
#define OLED_I2C_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define THERMISTOR_PIN A1
#define SERVO_BUTTON_PIN_0 8
#define SERVO_BUTTON_PIN_180 9
#define SERVO_MOVE_DELAY 2000  // 2 seconds delay

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Servo myservo;

bool ledState = false;
int ledBrightness = 255;
bool lastButtonState = false;
bool manualMode = false;
bool manualServoMode = false;
int servoPos = 0;
bool lastServoButtonState0 = false;
bool lastServoButtonState180 = false;
unsigned long lastServoMoveTime = 0;
unsigned long buttonPressTime = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SERVO_BUTTON_PIN_0, INPUT_PULLUP);
  pinMode(SERVO_BUTTON_PIN_180, INPUT_PULLUP);

  myservo.attach(10);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.begin(9600);
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);  // Infinite loop if display fails
  }
  display.clearDisplay();
  Serial.begin(9600);
  Serial.println(F("Setup completed"));

  float temp = readTemperature();
  if (temp <= 30) {
    myservo.write(0);  // Set the initial servo position to 0
    servoPos = 0;
  } else {
    myservo.write(180);  // Set the initial servo position to 180 if the temperature is above 30 degrees
    servoPos = 180;
  }
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);
  Serial.print("LDR Value: ");
  Serial.println(ldrValue);

  bool currentButtonState = digitalRead(BUTTON_PIN) == LOW;

  if (currentButtonState && !lastButtonState) {
    delay(20);  // Delay
    manualMode = !manualMode;
    Serial.print("Manual Mode: ");
    Serial.println(manualMode ? "ON" : "OFF");
    if (manualMode) {
        ledState = !ledState;
        if (ledState && ledBrightness <= 10) ledBrightness = 255;  // Reset brightness if switching to ON in manual mode
    } else {
        ledState = (ldrValue >= 200);
        ledBrightness = 255;  // Reset brightness when switching to auto mode
    }
  }

  if (manualMode && currentButtonState) {
    unsigned long pressDuration = millis() - buttonPressTime;
    ledBrightness = max(10, 255 - constrain(pressDuration / 10, 0, 245));  // Ensure minimum brightness is 10
    ledState = true;
  } else {
    buttonPressTime = millis();
  }
  lastButtonState = currentButtonState;

  if (!manualMode) {
    ledState = (ldrValue >= 200);
  }

  // Servo button logic for 0 (Closed) position
  bool currentServoButtonState0 = digitalRead(SERVO_BUTTON_PIN_0) == LOW;
  if (currentServoButtonState0 && !lastServoButtonState0) {
    delay(20);  // Delay
    if (!digitalRead(SERVO_BUTTON_PIN_0)) {
      manualServoMode = !manualServoMode;
      Serial.print("Manual Servo Mode (0): ");
      Serial.println(manualServoMode ? "ON" : "OFF");
      if (manualServoMode) {
        myservo.write(0);
        servoPos = 0;
      }
    }
  }
  lastServoButtonState0 = currentServoButtonState0;

  // Servo button logic for 180 (Open) position
  bool currentServoButtonState180 = digitalRead(SERVO_BUTTON_PIN_180) == LOW;
  if (currentServoButtonState180 && !lastServoButtonState180) {
    delay(20);  // Delay
    if (!digitalRead(SERVO_BUTTON_PIN_180)) {
      manualServoMode = !manualServoMode;
      Serial.print("Manual Servo Mode (180): ");
      Serial.println(manualServoMode ? "ON" : "OFF");
      if (manualServoMode) {
        myservo.write(180);
        servoPos = 180;
      }
    }
  }
  lastServoButtonState180 = currentServoButtonState180;

  if (!manualServoMode) {
    float temp = readTemperature();
    unsigned long currentMillis = millis();
    if ((currentMillis - lastServoMoveTime) >= SERVO_MOVE_DELAY) {
      if (temp > 30 && servoPos != 180) {
        myservo.write(180);
        servoPos = 180;
        lastServoMoveTime = currentMillis;
      } else if (temp <= 30 && servoPos != 0) {
        myservo.write(0);
        servoPos = 0;
        lastServoMoveTime = currentMillis;
      }
    }
  }

  analogWrite(LED_PIN, ledState ? ledBrightness : 0);
  displayStatus();
}

float readTemperature() {
  int Vo = analogRead(THERMISTOR_PIN);

  float R1 = 10000;
  float logR2, R2, T;
  float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

  R2 = R1 / ((1023.0 / (float)Vo) - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));

  T = T - 273.15 + 0.5;  // Temperature calibration 

  Serial.print("Calculated Temperature: ");
  Serial.println(T);

  return T;
}

void displayStatus() {
  float temp = readTemperature();
  int brightnessPercentage = (ledBrightness * 100) / 255;
  
  Serial.print("LED Brightness Percentage: ");
  Serial.println(brightnessPercentage);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println(ledState ? "LED: ON" : "LED: OFF");
  display.print("Dim Level: ");
  display.print(brightnessPercentage);
  display.println("%");
  display.println(manualMode ? "LED Mode: Manual" : "LED Mode: Auto");
  display.println(manualServoMode ? "Hatch Mode: Manual" : "Hatch Mode: Auto");
  display.print("Hatch Pos: ");
  display.println(servoPos == 0 ? "Closed" : "Open");
  display.print("Temp: ");
  display.print(temp, 1);
  display.println(" C");

  display.display();
}
