#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <LiquidCrystal_I2C.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <DHT.h>

// ==================== PIN CONFIGURATION ====================
#define RELAY_PIN 2        // Active HIGH relay pin for motor
#define MOISTURE_PIN A0    // Analog moisture sensor
#define MANUAL_BUTTON 5    // Manual button pin - LOW when pressed
#define DHTPIN 4           // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11      // DHT 11

// ==================== WIFI & FIREBASE CONFIG ====================
#define WIFI_SSID "home"
#define WIFI_PASSWORD "idontknow4321"

#define API_KEY "AIzaSyA8waCa8N0wsmhxHaN_7czChUIDS-o77t8"
#define DATABASE_URL "https://smart-irrigation-ef8df-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Using Anonymous Authentication to match Webapp
#define USER_EMAIL ""
#define USER_PASSWORD ""

// ==================== OBJECTS ====================
FirebaseData fbdo;
FirebaseData fbdo_control; // Use separate object for polling to prevent TCP collisions
FirebaseAuth auth;
FirebaseConfig config;
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// ==================== VARIABLES ====================
bool motorState = false;
bool manualMode = false;
float moisture = 0;
float temperature = 0.0;
float humidity = 0.0;
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
const char* tempPath = "/sensorData/temperature";
const char* humPath = "/sensorData/humidity";
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
  dht.begin();
  
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
    checkMotorControl(); // ✅ MUST run FIRST: read remote commands before auto-logic
    readSensors();       // This calls runAutoLogic() — now has up-to-date mode/motor state
    updateFirebase();
  }
  
  pumpSafetyCheck();
  updateLCD();
  delay(20); // Small delay
}

// We need an additional variable for stabilized button state to fix the debounce issue
static bool stabilizedButtonState = HIGH;

void handleButton() {
  bool reading = digitalRead(MANUAL_BUTTON);
  
  if (reading != lastButtonState) {
    buttonPressTime = millis();
  }
  
  if ((millis() - buttonPressTime) > debounceDelay) {
    // If the reading has been stable longer than debounce delay
    if (reading != stabilizedButtonState) {
      stabilizedButtonState = reading;
      
      // Only do action when the button goes LOW (pressed)
      if (stabilizedButtonState == LOW) {
        manualMode = !manualMode;
        motorState = manualMode;
        digitalWrite(RELAY_PIN, motorState ? HIGH : LOW);
        if (manualMode) pumpStartTime = millis();
        
        // Update Firebase immediately on physical button press
        if (Firebase.ready()) {
          Firebase.RTDB.setBool(&fbdo, manualModePath, manualMode);
          Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
        }
        
        Serial.printf("🔘 MANUAL: %s\n", manualMode ? "ON" : "OFF");
      }
    }
  }
  lastButtonState = reading;
}

void readSensors() {
  int raw = analogRead(MOISTURE_PIN);
  moisture = 100.0 * (4095.0 - raw) / 4095.0;
  
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("⚠️ Failed to read from DHT sensor!");
  } else {
    Serial.printf("🌡️ Temp:%.1f°C | Hum:%.1f%%\n", temperature, humidity);
  }
  
  runAutoLogic();  // Evaluate auto irrigation logic after reading sensors
  
  Serial.printf("💧 M:%.0f%% | Pump:%s | Mode:%s\n", 
                moisture, motorState?"ON":"OFF", manualMode?"MAN":"AUTO");
}

// Extracted auto-logic so it can be called from multiple places
void runAutoLogic() {
  // AUTO PUMP: ON <30%, OFF >=30%
  if (!manualMode) {
    if (moisture < 30.0 && !motorState) {
      motorState = true;
      digitalWrite(RELAY_PIN, HIGH);
      pumpStartTime = millis();
      Serial.println("🚀 AUTO ON - DRY!");
      if (Firebase.ready()) Firebase.RTDB.setBool(&fbdo, motorPath, motorState);
    } 
    else if (moisture >= 30.0 && motorState) {
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
  
  if (!isnan(temperature)) {
    Firebase.RTDB.setFloat(&fbdo, tempPath, temperature);
    Serial.printf("☁️ sensorData/temp:%.1f ✅\n", temperature);
  }
  if (!isnan(humidity)) {
    Firebase.RTDB.setFloat(&fbdo, humPath, humidity);
    Serial.printf("☁️ sensorData/hum:%.1f ✅\n", humidity);
  }
  
  Serial.printf("☁️ sensorData/moisture:%.0f ✅\n", moisture);
  Serial.printf("☁️ metrics/greenScore:%d ✅\n", (int)moisture);
}

void checkMotorControl() {
  // ===== STEP 1: Check if Web App toggled Auto/Manual mode =====
  if (Firebase.ready() && Firebase.RTDB.getBool(&fbdo_control, manualModePath)) {
    bool fbMode = fbdo_control.boolData();
    if (fbMode != manualMode) {
      manualMode = fbMode;
      Serial.printf("☁️ FB MODE CHANGE: %s\n", manualMode ? "MANUAL" : "AUTO");
      
      if (manualMode) {
        // Switching TO MANUAL: stop motor, user must explicitly turn ON
        motorState = false;
        digitalWrite(RELAY_PIN, LOW);
        Firebase.RTDB.setBool(&fbdo_control, motorPath, false);
        Serial.println("✋ MANUAL mode — motor OFF, awaiting user command");
      } else {
        // Switching TO AUTO: immediately evaluate auto-logic
        Serial.println("🤖 AUTO activated from webapp — evaluating now...");
        // runAutoLogic() will be called by readSensors() right after this
      }
    }
  }

  // ===== STEP 2: Check if Web App updated the motor state =====
  if (Firebase.ready() && Firebase.RTDB.getBool(&fbdo_control, motorPath)) {
    bool fbCmd = fbdo_control.boolData();
    if (fbCmd != motorState) {
      if (manualMode) {
        // In manual mode: obey web app motor commands directly
        motorState = fbCmd;
        digitalWrite(RELAY_PIN, motorState ? HIGH : LOW);
        if (motorState) pumpStartTime = millis();
        Serial.printf("☁️ FB MANUAL CMD: %s\n", motorState ? "ON" : "OFF");
      } else {
        // In auto mode: ignore direct motor toggle from web, auto-logic controls it
        // Re-sync Firebase to match the auto-decided state
        Firebase.RTDB.setBool(&fbdo_control, motorPath, motorState);
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