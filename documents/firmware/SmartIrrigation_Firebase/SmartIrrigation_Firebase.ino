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
#define WIFI_SSID "lysamma"
#define WIFI_PASSWORD "12345678"

#define API_KEY "AIzaSyA8waCa8N0wsmhxHaN_7czChUIDS-o77t8"
#define DATABASE_URL "https://smart-irrigation-ef8df-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Using Anonymous Authentication to match Webapp
#define USER_EMAIL ""
#define USER_PASSWORD ""

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
const char* manualModePath = "/control/manualMode";
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
  
  // Authenticate Anonymously
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("✅ Auth OK");
  } else {
    Serial.printf("❌ Auth Error: %s\n", config.signer.signupError.message.c_str());
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
    checkMotorControl(); // Poll remote commands every 3 seconds (not every loop!)
  }
  
  pumpSafetyCheck();
  updateLCD();
  delay(20); // Small delay
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
      
      // Update Firebase immediately on physical button press
      Firebase.RTDB.setBool(&fbdo, manualModePath, manualMode);
      Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
      
      Serial.printf("🔘 MANUAL: %s\n", manualMode ? "ON" : "OFF");
    }
  }
  lastButtonState = buttonState;
}

void readSensors() {
  int raw = analogRead(MOISTURE_PIN);
  moisture = 100.0 * (4095.0 - raw) / 4095.0;
  
  runAutoLogic();  // Evaluate auto irrigation logic after reading sensors
  
  Serial.printf("💧 M:%.0f%% | Pump:%s | Mode:%s\n", 
                moisture, motorState?"ON":"OFF", manualMode?"MAN":"AUTO");
}

// Extracted auto-logic so it can be called from multiple places
void runAutoLogic() {
  // AUTO PUMP: ON <5%, OFF >70%
  if (!manualMode) {
    if (moisture <= 5.0 && !motorState) {
      motorState = true;
      digitalWrite(RELAY_PIN, HIGH);
      pumpStartTime = millis();
      Serial.println("🚀 AUTO ON - DRY!");
      if (Firebase.ready()) Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
    } 
    else if (moisture >= 70.0 && motorState) {
      motorState = false;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("✅ AUTO OFF - WET!");
      if (Firebase.ready()) Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
    }
  }
  // MANUAL MODE SAFETY: Cut off motor if soil is saturated even in manual mode
  else if (manualMode && motorState && moisture >= 90.0) {
    motorState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🛑 MANUAL SAFETY: Moisture >=90%, motor OFF!");
    if (Firebase.ready()) {
      Firebase.RTDB.setBool(&fbdo, motorPath, false);
    }
  }
}

void updateFirebase() {
  if (!Firebase.ready()) {
    Serial.println("❌ Firebase offline");
    return;
  }
  
  // ✅ YOUR EXACT PATHS (ONLY SENSORS periodically)
  Firebase.RTDB.setFloat(&fbdo, moisturePath, moisture);
  Firebase.RTDB.setInt(&fbdo, greenScorePath, (int)moisture);  // GREEN SCORE!
  
  Serial.printf("☁️ sensorData/moisture:%.0f ✅\n", moisture);
  Serial.printf("☁️ metrics/greenScore:%d ✅\n", (int)moisture);
}

void checkMotorControl() {
  // ===== STEP 1: Check if Web App toggled Auto/Manual mode =====
  // Must check mode FIRST, before motor commands
  if (Firebase.ready() && Firebase.RTDB.getBool(&fbdo, manualModePath)) {
    bool fbMode = fbdo.boolData();
    if (fbMode != manualMode) {
      manualMode = fbMode;
      Serial.printf("☁️ FB MODE CHANGE: %s\n", manualMode ? "MANUAL" : "AUTO");
      
      // When switching TO AUTO mode, immediately run auto-logic
      if (!manualMode) {
        Serial.println("🤖 AUTO activated from webapp — evaluating now...");
        runAutoLogic();  // Immediately decide pump state based on moisture
      }
    }
  }

  // ===== STEP 2: Check if Web App updated the motor state =====
  // Only treat as manual override if we are ALREADY in manual mode
  if (Firebase.ready() && Firebase.RTDB.getBool(&fbdo, motorPath)) {
    bool fbCmd = fbdo.boolData();
    if (fbCmd != motorState) {
      if (manualMode) {
        // In manual mode: obey web app motor commands directly
        motorState = fbCmd;
        digitalWrite(RELAY_PIN, motorState);
        if (motorState) pumpStartTime = millis();
        Serial.printf("☁️ FB MANUAL CMD: %s\n", motorState ? "ON" : "OFF");
      } else {
        // In auto mode: ignore direct motor toggle from web, auto-logic controls it
        // Re-sync Firebase to match the auto-decided state
        Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
        Serial.println("☁️ FB motor cmd IGNORED (AUTO mode active)");
      }
    }
  }
}

void pumpSafetyCheck() {
  if (motorState && (millis() - pumpStartTime > maxPumpTime)) {
    motorState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🛑 SAFETY: 5min timeout");
    if (Firebase.ready()) {
      Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
      Firebase.RTDB.setBool(&fbdo, manualModePath, false); // Switch back to auto
      manualMode = false;
    }
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