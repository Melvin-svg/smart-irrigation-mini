# 🌱 Smart Irrigation System — Wiring Guide

## Components Required

| # | Component | Quantity | Notes |
|---|-----------|----------|-------|
| 1 | ESP32 Dev Board | 1 | Any variant (DevKitC, NodeMCU-32S, etc.) |
| 2 | DHT11 Sensor | 1 | (or DHT22 for better accuracy) |
| 3 | Capacitive Soil Moisture Sensor v1.2 | 1 | Analog output |
| 4 | 5V Relay Module | 1 | Single channel, active LOW |
| 5 | L298N Motor Driver | 1 | Dual H-Bridge |
| 6 | Water Pump (5V or 12V) | 1 | Submersible DC motor pump |
| 7 | 10kΩ Resistor | 1 | Pull-up for DHT11 data line |
| 8 | Breadboard + Jumper Wires | — | Male-to-male & male-to-female |
| 9 | 12V Power Supply | 1 | For L298N (if using 12V pump) |
| 10 | USB Cable (Micro-USB) | 1 | For programming & powering ESP32 |

---

## Pin Mapping Table

| ESP32 GPIO | Connects To | Wire Color (Suggested) |
|------------|-------------|----------------------|
| **GPIO 4** | DHT11 DATA pin | 🟢 Green |
| **GPIO 34** | Soil Moisture Sensor AOUT | 🟡 Yellow |
| **GPIO 26** | Relay Module IN | 🔵 Blue |
| **GPIO 27** | L298N IN1 | 🟠 Orange |
| **GPIO 14** | L298N IN2 | 🟤 Brown |
| **GPIO 12** | L298N ENA (PWM) | 🟣 Purple |
| **3.3V** | DHT11 VCC, Soil Sensor VCC | 🔴 Red |
| **5V (VIN)** | Relay Module VCC | 🔴 Red |
| **GND** | Common Ground (all modules) | ⚫ Black |

---

## Wiring Diagram (Text)

```
                          ┌──────────────────┐
                          │    ESP32 Board    │
                          │                  │
         DHT11            │  GPIO 4  ◄───────┤──── DHT11 DATA (+ 10kΩ pull-up to 3.3V)
         Sensor           │  3.3V    ────────┤──── DHT11 VCC
                          │  GND     ────────┤──── DHT11 GND
                          │                  │
         Soil Moisture    │  GPIO 34 ◄───────┤──── Soil Sensor AOUT
         Sensor           │  3.3V    ────────┤──── Soil Sensor VCC
                          │  GND     ────────┤──── Soil Sensor GND
                          │                  │
         Relay Module     │  GPIO 26 ────────┤──── Relay IN
                          │  5V/VIN  ────────┤──── Relay VCC
                          │  GND     ────────┤──── Relay GND
                          │                  │
         L298N Motor      │  GPIO 27 ────────┤──── L298N IN1
         Driver           │  GPIO 14 ────────┤──── L298N IN2
                          │  GPIO 12 ────────┤──── L298N ENA
                          │  GND     ────────┤──── L298N GND
                          └──────────────────┘

         ┌───────────────────────────────┐
         │         L298N Module          │
         │                               │
         │  12V IN ◄── 12V Power Supply  │
         │  GND    ◄── Power Supply GND  │
         │  5V OUT ──► (can power ESP32) │
         │                               │
         │  OUT1 ──┐                     │
         │  OUT2 ──┤──► Water Pump       │
         └─────────┴─────────────────────┘

         ┌──────────────────────────┐
         │     Relay Module         │
         │                          │
         │  COM  ──── 12V Supply +  │
         │  NO   ──── L298N 12V IN  │
         │  NC   ──── (not used)    │
         └──────────────────────────┘
```

---

## Detailed Wiring Instructions

### 1️⃣ DHT11 Temperature & Humidity Sensor

The DHT11 has **3 pins** (some modules have 4, with pin 3 unused):

| DHT11 Pin | Connect To |
|-----------|-----------|
| VCC (+) | ESP32 **3.3V** |
| DATA | ESP32 **GPIO 4** + 10kΩ resistor to 3.3V |
| GND (−) | ESP32 **GND** |

> **Note:** The 10kΩ pull-up resistor goes between the DATA pin and VCC (3.3V). Some DHT11 modules have this built-in on a breakout board.

---

### 2️⃣ Capacitive Soil Moisture Sensor

| Soil Sensor Pin | Connect To |
|----------------|-----------|
| VCC | ESP32 **3.3V** |
| GND | ESP32 **GND** |
| AOUT | ESP32 **GPIO 34** |

> **Important:** GPIO 34 is an input-only ADC pin on ESP32, which is ideal for analog sensors.  
> **Calibration:** Note the raw ADC values when the sensor is dry (in air) and wet (in water). Update `SOIL_DRY_VALUE` and `SOIL_WET_VALUE` in the code.

---

### 3️⃣ 5V Relay Module

| Relay Pin | Connect To |
|-----------|-----------|
| VCC | ESP32 **5V / VIN** |
| GND | ESP32 **GND** |
| IN | ESP32 **GPIO 26** |
| COM | 12V Power Supply **positive (+)** |
| NO | L298N **12V input** |

> **How it works:** When GPIO 26 goes LOW, the relay switches ON and connects the 12V supply to the L298N, powering the pump.

---

### 4️⃣ L298N Motor Driver

| L298N Pin | Connect To |
|-----------|-----------|
| 12V IN | Relay **NO** terminal |
| GND | 12V Power Supply **GND** + ESP32 **GND** |
| IN1 | ESP32 **GPIO 27** |
| IN2 | ESP32 **GPIO 14** |
| ENA | ESP32 **GPIO 12** (remove jumper cap!) |
| OUT1 | Water Pump **wire 1** |
| OUT2 | Water Pump **wire 2** |

> **Critical:** Remove the jumper cap on the ENA pins of the L298N to allow PWM speed control from GPIO 12.  
> **GND must be shared** between the ESP32, L298N, and the 12V power supply.

---

### 5️⃣ Water Pump

| Pump Wire | Connect To |
|-----------|-----------|
| Wire 1 | L298N **OUT1** |
| Wire 2 | L298N **OUT2** |

> The pump direction is controlled by IN1/IN2. Since it's a simple pump, direction doesn't matter — just verify it pumps correctly.

---

## Power Supply Notes

| Power Rail | Source | Powers |
|-----------|--------|--------|
| **3.3V** | ESP32 onboard regulator | DHT11, Soil Moisture sensor |
| **5V** | USB or L298N 5V OUT | ESP32 (via VIN), Relay VCC |
| **12V** | External adapter/battery | L298N → Water Pump |

> ⚠️ **WARNING:** Never power the water pump directly from the ESP32. Always use the external 12V supply through the relay + L298N.

---

## Common Issues & Troubleshooting

| Problem | Solution |
|---------|----------|
| DHT11 reads NaN | Check 10kΩ pull-up resistor; verify VCC is 3.3V |
| Soil moisture always 0% or 100% | Recalibrate `SOIL_DRY_VALUE` and `SOIL_WET_VALUE` |
| Relay clicks but pump doesn't run | Check 12V supply, L298N ENA jumper removed |
| ESP32 keeps rebooting | Insufficient power — use separate 5V supply for ESP32 |
| Firebase connection fails | Verify WiFi credentials, API key, and database URL |
| Pump runs in wrong direction | Swap OUT1/OUT2 wires on the L298N |

---

## Arduino IDE Setup

1. **Install ESP32 Board:**
   - Go to `File → Preferences`
   - Add Board Manager URL: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Go to `Tools → Board → Boards Manager` → search "ESP32" → Install

2. **Install Libraries** (via `Sketch → Include Library → Manage Libraries`):
   - `Firebase ESP Client` by mobizt
   - `DHT sensor library` by Adafruit
   - `Adafruit Unified Sensor` by Adafruit

3. **Board Settings:**
   - Board: `ESP32 Dev Module`
   - Upload Speed: `115200`
   - Flash Frequency: `80MHz`
   - Port: Select your COM port

4. **Upload:**
   - Edit the `#define` values for WiFi and Firebase credentials
   - Click Upload → Open Serial Monitor at `115200` baud
