# 🌱 Smart Irrigation System — PRD

## 1. Overview

**Product Name:** Smart Irrigation System  
**Platform:** Web App (React) + ESP32 IoT Device  
**Backend:** Firebase Realtime Database  

This system monitors soil moisture and environmental humidity using sensors connected to an ESP32, and allows users to:
- View real-time data
- Manually control irrigation
- Track a sustainability metric called **Green Score**

---

## 2. Goals

### Primary Goals
- Enable real-time monitoring of irrigation conditions
- Provide manual irrigation control via web app
- Automate data syncing between ESP32 and Firebase
- Display sustainability insights (Green Score)

### Secondary Goals
- Improve water efficiency
- Provide simple, intuitive UI
- Allow debugging via Serial Monitor

---

## 3. Users

- Farmers / home gardeners
- IoT hobbyists
- Sustainability-focused users

---

## 4. System Architecture

### Hardware Layer
- ESP32  
- Soil Moisture Sensor  
- Humidity Sensor (DHT11/DHT22)  
- 5V Relay  
- Water Pump  
- L298N Motor Driver  

### Data Flow
1. Sensors → ESP32 reads values  
2. ESP32 → Sends data to Firebase  
3. Firebase → Syncs with React Web App  
4. Web App → Sends manual control commands  
5. ESP32 → Executes irrigation  

---

## 5. Features

### 5.1 Real-Time Monitoring
- Soil moisture (%)  
- Humidity (%)  
- Pump status (ON/OFF)  
- Last updated timestamp  

---

### 5.2 Manual Irrigation Control
- Toggle button:
  - ON → Activate pump  
  - OFF → Stop pump  
- Instant sync with ESP32 via Firebase  

---

### 5.3 Green Score Calculation

A sustainability metric (0–100) based on:

#### Inputs:
- Soil moisture  
- Humidity  
- Pump usage duration  

#### Formula (example):
moistureScore = map(moisture, 0–100 → 0–50)
humidityScore = map(humidity, 0–100 → 0–30)
waterPenalty = pumpOnTime (seconds) * 0.1

greenScore = moistureScore + humidityScore - waterPenalty
greenScore = constrain(0, 100)


---

### Display:
- Web app (dashboard)  
- Serial Monitor (ESP32 logs)  

---

## 6. Firebase Structure

```json
{
  "sensorData": {
    "moisture": 45,
    "humidity": 60,
    "timestamp": 1710000000
  },
  "control": {
    "pump": true
  },
  "metrics": {
    "greenScore": 72
  }
}

7. React Web App
7.1 Tech Stack
React (Vite or CRA)
Firebase SDK
Chart.js or Recharts (optional)
7.2 Pages / Components
Dashboard
Moisture Card
Humidity Card
Pump Status Indicator
Green Score Meter
Control Panel
Toggle Switch for Pump
Manual irrigation button
Logs (Optional)
Historical data chart
7.3 UI Requirements
Responsive design
Clean card-based layout
Color indicators:
Green → optimal
Yellow → moderate
Red → critical
8. ESP32 Responsibilities
Sensor Handling
Read moisture sensor (analog)
Read humidity sensor (digital)
Firebase Integration
Push sensor data every 5–10 seconds
Listen for control changes (pump ON/OFF)
Pump Control
Activate relay + L298N driver
Serial Monitor Output (Example)
Moisture: 45%
Humidity: 60%
Pump: ON
Green Score: 72
9. Green Score Logic
ESP32 Version
Calculate locally for Serial Monitor
Push to Firebase
Web App Version
Recalculate OR read from Firebase
Display visually (progress bar / gauge)
10. Non-Functional Requirements
Real-time updates (<2s latency)
Reliable Firebase sync
Low power consumption (ESP32)
Secure Firebase rules
11. Firebase Rules (Basic)
{
  "rules": {
    "sensorData": {
      ".read": true,
      ".write": true
    },
    "control": {
      ".read": true,
      ".write": true
    }
  }
}
12. Future Enhancements
Automatic irrigation logic (AI-based)
Weather API integration
Mobile app (React Native)
Alerts/notifications
Multi-zone irrigation
13. Success Metrics
Reduced water usage
Stable sensor readings
Low latency control response
User engagement with dashboard
14. Risks
Sensor inaccuracies
Network instability
Firebase overuse limits
Power issues in pump control
15. Milestones
Hardware setup + ESP32 coding
Firebase integration
React dashboard UI
Control sync
Green Score implementation
Testing & optimization