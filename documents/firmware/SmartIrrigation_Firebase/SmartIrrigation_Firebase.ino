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
// ⚠️ CHANGE THESE TO YOUR VALUES ⚠️
#define WIFI_SSID "home"
#define WIFI_PASSWORD "idontknow4321"

#define API_KEY "AIzaSyA8waCa8N0wsmhxHaN_7czChUIDS-o77t8"
#define DATABASE_URL "https://smart-irrigation-ef8df-default-rtdb.asia-southeast1.firebasedatabase.app/"

#define USER_EMAIL "admin@irrigation.com"      // ← YOUR EMAIL
#define USER_PASSWORD "password123"            // ← YOUR PASSWORD

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
const long interval = 3000;  // Read every 3 seconds
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
const unsigned long debounceDelay = 200;

// SAFETY: Pump timer protection
unsigned long pumpStartTime = 0;
const unsigned long maxPumpTime = 300000;  // 5 minutes max

const char* moisturePath = "/sensors/moisture";
const char* motorPath = "/control/motor";

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(MANUAL_BUTTON, INPUT_PULLUP);
  
  // I2C LCD
  Wire.begin(21, 22);  // SDA=21, SCL=22
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("ESP32 Irrigation");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);
  
  connectWiFi();
  initFirebase();
  
  lcd.clear();
  lcd.print("READY! Btn=Manual");
  lcd.setCursor(0, 1);
  lcd.print("Auto:5%-70%");
  delay(2000);
}

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi: ");
  lcd.clear();
  lcd.print("WiFi Connecting");
  
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
    Serial.println("\n✅ WiFi OK: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.print("WiFi OK!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    delay(2000);
  }
}

void initFirebase() {
  Serial.println("\n🔥 Initializing Firebase...");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  // ✅ CRITICAL: Authenticate
  Serial.printf("Authenticating %s...\n", USER_EMAIL);
  if (Firebase.signUp(&config, &auth, USER_EMAIL, USER_PASSWORD)) {
    Serial.println("✅ Auth OK");
  } else {
    Serial.printf("Auth error: %s\n", config.signer.signupError.message.c_str());
  }
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.reconnectNetwork(true);
  
  // Wait for token
  Serial.print("Token ready");
  int tokenWait = 0;
  while (auth.token.uid == "" && tokenWait < 30) {
    Serial.print(".");
    delay(1000);
    tokenWait++;
  }
  Serial.println("\n✅ Firebase LIVE!");
  
  lcd.clear();
  lcd.print("Firebase LIVE!");
  delay(1500);
}

void loop() {
  unsigned long currentMillis = millis();
  
  handleButton();
  
  // Read sensors every 3 seconds
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    readSensors();
    updateFirebase();
  }
  
  checkMotorControl();
  pumpSafetyCheck();  // Safety timer
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
      
      if (manualMode) {
        pumpStartTime = millis();  // Reset timer in manual
      }
      
      Serial.printf("🔘 MANUAL: %s | Motor: %s\n", 
                    manualMode ? "ON" : "OFF", motorState ? "ON" : "OFF");
    }
  }
  lastButtonState = buttonState;
}

void readSensors() {
  int raw = analogRead(MOISTURE_PIN);
  moisture = 100.0 * (4095.0 - raw) / 4095.0;  // 0% dry, 100% wet
  
  // ✅ FIXED AUTO LOGIC: ON at 5%, OFF at 70%
  if (!manualMode) {
    if (moisture <= 5.0) {
      motorState = true;
      digitalWrite(RELAY_PIN, HIGH);
      pumpStartTime = millis();  // Start safety timer
      Serial.println("🚀 AUTO ON - SOIL DRY!");
    } 
    else if (moisture >= 70.0) {
      motorState = false;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("✅ AUTO OFF - SOIL WET!");
    }
  }
  
  Serial.printf("💧 M:%.0f%% RAW:%4d | Pump:%s | Mode:%s\n", 
                moisture, raw, motorState?"ON":"OFF", manualMode?"MAN":"AUTO");
}

void updateFirebase() {
  if (!Firebase.ready()) {
    Serial.println("❌ Firebase NOT ready");
    return;
  }
  
  // Moisture
  if (Firebase.RTDB.setFloat(&fbdo, moisturePath, moisture)) {
    Serial.printf("☁️ M:%.0f%% OK\n", moisture);
  }
  
  // Motor
  if (Firebase.RTDB.setBool(&fbdo, motorPath, motorState)) {
    Serial.printf("☁️ Pump:%s OK\n", motorState?"ON":"OFF");
  }
}

void checkMotorControl() {
  if (manualMode || !Firebase.ready()) return;
  
  if (Firebase.RTDB.getBool(&fbdo, motorPath)) {
    bool fbCmd = fbdo.boolData();
    if (fbCmd != motorState) {
      motorState = fbCmd;
      digitalWrite(RELAY_PIN, motorState);
      pumpStartTime = millis();
      Serial.printf("☁️ FB CMD: %s\n", motorState ? "ON" : "OFF");
    }
  }
}

void pumpSafetyCheck() {
  if (motorState && (millis() - pumpStartTime > maxPumpTime)) {
    motorState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("🛑 SAFETY: Pump timeout 5min!");
  }
}

void updateLCD() {
  static unsigned long lcdMillis = 0;
  
  if (millis() - lcdMillis < 2500) return;
  lcdMillis = millis();
  
  lcd.clear();
  
  // Line 1: Moisture + Pump Status
  lcd.setCursor(0, 0);
  lcd.print("M:");
  lcd.print((int)moisture);
  lcd.print("% ");
  lcd.print(motorState ? "PUMP" : "IDLE");
  
  // Line 2: Mode + Firebase + Green Score
  lcd.setCursor(0, 1);
  lcd.print(manualMode ? "MANUAL" : "AUTO ");
  lcd.print(Firebase.ready() ? "FB:OK" : "FB:NO");
  lcd.print(" G:");
  lcd.print((int)moisture);  // GREEN SCORE
  
  // Green indicator (if >70%)
  if (moisture >= 70) {
    lcd.print("✅");
  }
}