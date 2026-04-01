#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ==================== PIN CONFIGURATION ====================
#define RELAY_PIN 2        // Active HIGH relay pin for motor
#define MOISTURE_PIN A0    // Analog moisture sensor
#define MANUAL_BUTTON 5    // Manual button pin - LOW when pressed

// ==================== WIFI & FIREBASE CONFIG ====================
#define WIFI_SSID "home"
#define WIFI_PASSWORD "idontknow4321"

#define API_KEY "AIzaSyA8waCa8N0wsmhxHaN_7czChUIDS-o77t8"
#define DATABASE_URL "https://smart-irrigation-ef8df-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define USER_EMAIL "admin@irrigation.com"      // CHANGE TO YOUR EMAIL
#define USER_PASSWORD "password123"            // CHANGE TO YOUR PASSWORD

// ==================== OBJECTS ====================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==================== VARIABLES ====================
bool motorState = false;
bool manualMode = false;
float moisture = 0;
unsigned long previousMillis = 0;
const long interval = 3000;
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
const unsigned long debounceDelay = 200;

// SAFETY: Pump timer
unsigned long pumpStartTime = 0;
const unsigned long maxPumpTime = 300000;  // 5 minutes

// ✅ MATCHES YOUR FIREBASE STRUCTURE
const char* moisturePath = "/sensorData/moisture";
const char* motorPath = "/control/motor";
const char* greenScorePath = "/metrics/greenScore";

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(MANUAL_BUTTON, INPUT_PULLUP);
  
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Irrigation");
  lcd.setCursor(0, 1);
  lcd.print("Booting...");
  delay(2000);
  
  connectWiFi();
  initFirebase();
  
  lcd.clear();
  lcd.print("LIVE! Btn=Manual");
  delay(1500);
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi: ");
  lcd.clear();
  lcd.print("WiFi...");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(0, 1);
    lcd.print("Try ");
    lcd.print(attempts);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.print("WiFi OK!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    delay(2000);
  }
}

void initFirebase() {
  Serial.println("\n🔥 Firebase...");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // Authenticate
  if (Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD)) {
    Serial.println("✅ Auth OK");
  }
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  Serial.print("Token");
  while (auth.token.uid == "") {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\n✅ Firebase LIVE!");
  
  lcd.clear();
  lcd.print("Firebase LIVE!");
  delay(1500);
}

void loop() {
  unsigned long currentMillis = millis();
  
  handleButton();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensors();
    updateFirebase();
  }
  
  checkMotorControl();
  pumpSafetyCheck();
  updateLCD();
  delay(100);
}

void handleButton() {
  bool buttonState = digitalRead(MANUAL_BUTTON);
  
  if (buttonState != lastButtonState) {
    buttonPressTime = millis();
  }
  
  if ((millis() - buttonPressTime) > debounceDelay) {
    if (buttonState == LOW && lastButtonState == HIGH) {
      manualMode = !manualMode;
      motorState = manualMode;
      digitalWrite(RELAY_PIN, motorState);
      if (manualMode) pumpStartTime = millis();
      
      Serial.printf("🔘 MANUAL: %s\n", manualMode ? "ON" : "OFF");
    }
  }
  lastButtonState = buttonState;
}

void readSensors() {
  int raw = analogRead(MOISTURE_PIN);
  moisture = 100.0 * (4095.0 - raw) / 4095.0;
  
  // AUTO PUMP: ON <5%, OFF >70%
  if (!manualMode) {
    if (moisture <= 5.0) {
      motorState = true;
      digitalWrite(RELAY_PIN, HIGH);
      pumpStartTime = millis();
      Serial.println("🚀 AUTO ON - DRY!");
    } 
    else if (moisture >= 70.0) {
      motorState = false;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("✅ AUTO OFF - WET!");
    }
  }
  
  Serial.printf("💧 M:%.0f%% | Pump:%s | Mode:%s\n", 
                moisture, motorState?"ON":"OFF", manualMode?"MAN":"AUTO");
}

void updateFirebase() {
  if (!Firebase.ready()) {
    Serial.println("❌ Firebase offline");
    return;
  }
  
  // ✅ YOUR EXACT PATHS
  Firebase.RTDB.setFloat(&fbdo, moisturePath, moisture);
  Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
  Firebase.RTDB.setInt(&fbdo, greenScorePath, (int)moisture);  // GREEN SCORE!
  
  Serial.printf("☁️ sensorData/moisture:%.0f ✅\n", moisture);
  Serial.printf("☁️ control/motor:%s ✅\n", motorState?"ON":"OFF");
  Serial.printf("☁️ metrics/greenScore:%d ✅\n", (int)moisture);
}

void checkMotorControl() {
  if (manualMode || !Firebase.ready()) return;
  
  if (Firebase.RTDB.getBool(&fbdo, motorPath)) {
    bool fbCmd = fbdo.boolData();
    if (fbCmd != motorState) {
      motorState = fbCmd;
      digitalWrite(RELAY_PIN, motorState);
      pumpStartTime = millis();
      Serial.printf("☁️ FB CMD: %s\n", motorState?"ON":"OFF");
    }
  }
}

void pumpSafetyCheck() {
  if (motorState && (millis() - pumpStartTime > maxPumpTime)) {
    motorState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🛑 SAFETY: 5min timeout");
  }
}

void updateLCD() {
  static unsigned long lcdMillis = 0;
  
  if (millis() - lcdMillis < 2500) return;
  lcdMillis = millis();
  
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("M:");
  lcd.print((int)moisture);
  lcd.print("% ");
  lcd.print(motorState ? "PUMP" : "IDLE");
  
  lcd.setCursor(0, 1);
  lcd.print(manualMode ? "MAN" : "AUTO");
  lcd.print(Firebase.ready() ? " FB:OK" : " FB:--");
  lcd.print(" G:");
  lcd.print((int)moisture);
}