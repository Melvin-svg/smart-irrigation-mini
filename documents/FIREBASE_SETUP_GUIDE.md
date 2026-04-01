# 🔥 Firebase Setup Guide — Smart Irrigation System

> Follow these steps **exactly** in order. By the end, you'll have your Firebase project ready and the credentials needed for both the ESP32 firmware and the web app.

---

## Step 1: Create a Firebase Project

1. Go to **[Firebase Console](https://console.firebase.google.com/)**
2. Click **"Create a project"** (or "Add project")
3. Enter project name: `smart-irrigation` (or any name you like)
4. **Disable** Google Analytics (not needed for this project) → Click **Create Project**
5. Wait for the project to be created → Click **Continue**

---

## Step 2: Set Up Realtime Database

1. In the left sidebar, click **Build → Realtime Database**
2. Click **"Create Database"**
3. Choose a location closest to you:
   - `asia-southeast1` (Singapore) — good for India
   - `us-central1` (Iowa) — default
4. Select **"Start in Test Mode"** → Click **Enable**

> ⚠️ **Test mode** allows open read/write for 30 days. We'll secure it later.

5. You'll see an empty database. Note the **Database URL** at the top — it looks like:
   ```
   https://smart-irrigation-xxxxx-default-rtdb.firebaseio.com/
   ```
   📋 **Copy this URL** — you'll need it for the ESP32 code.

---

## Step 3: Manually Create the Database Structure

In the Realtime Database console, click the **`+`** button next to your database root to add the following structure:

```
smart-irrigation-xxxxx-default-rtdb
├── sensorData
│   ├── moisture: 0
│   ├── humidity: 0
│   ├── temperature: 0
│   └── timestamp: 0
├── control
│   ├── pump: false
│   └── pumpStatus: false
└── metrics
    └── greenScore: 0
```

**How to add each node:**

1. Click **`+`** next to the root
2. Type `sensorData` as the key → click **`+`** again to add children:
   - Key: `moisture`, Value: `0` → click ✅
   - Key: `humidity`, Value: `0` → click ✅
   - Key: `temperature`, Value: `0` → click ✅
   - Key: `timestamp`, Value: `0` → click ✅
3. Go back to root → click **`+`**
4. Type `control` → add children:
   - Key: `pump`, Value: `false` (select type as boolean if available)
   - Key: `pumpStatus`, Value: `false`
5. Go back to root → click **`+`**
6. Type `metrics` → add child:
   - Key: `greenScore`, Value: `0`

---

## Step 4: Get Your Firebase API Key

1. Click the **⚙️ gear icon** (top-left) → **Project Settings**
2. Scroll down to **"Your apps"** section
3. Click the **Web icon `</>`** to register a web app
4. Enter app nickname: `smart-irrigation-web`
5. ✅ Check **"Also set up Firebase Hosting"** (optional, skip if you don't need hosting)
6. Click **Register app**
7. You'll see a code block with your Firebase config:

```javascript
const firebaseConfig = {
  apiKey: "AIzaSy.....................",         // ← YOUR API KEY
  authDomain: "smart-irrigation-xxxxx.firebaseapp.com",
  databaseURL: "https://smart-irrigation-xxxxx-default-rtdb.firebaseio.com/",
  projectId: "smart-irrigation-xxxxx",
  storageBucket: "smart-irrigation-xxxxx.appspot.com",
  messagingSenderId: "123456789012",
  appId: "1:123456789012:web:abcdef1234567890"
};
```

📋 **Copy this ENTIRE config block** — Save it somewhere safe, you'll need it for:
- **ESP32 firmware:** `apiKey` and `databaseURL`
- **Web app:** The full config object

8. Click **Continue to console**

---

## Step 5: Update Your ESP32 Firmware Credentials

Open `firmware/SmartIrrigation_Firebase.ino` and fill in these 4 values:

```cpp
// Wi-Fi Credentials
#define WIFI_SSID       "YourWiFiName"          // ← Your WiFi network name
#define WIFI_PASSWORD   "YourWiFiPassword"       // ← Your WiFi password

// Firebase Credentials (from Step 4)
#define FIREBASE_API_KEY      "AIzaSy..."        // ← apiKey from config
#define FIREBASE_DATABASE_URL "https://smart-irrigation-xxxxx-default-rtdb.firebaseio.com/"
```

---

## Step 6: Set Database Rules (Security)

1. In Firebase Console → **Realtime Database → Rules** tab
2. Replace the default rules with:

```json
{
  "rules": {
    "sensorData": {
      ".read": true,
      ".write": true
    },
    "control": {
      ".read": true,
      ".write": true
    },
    "metrics": {
      ".read": true,
      ".write": true
    }
  }
}
```

3. Click **Publish**

> ⚠️ These are **open rules** for development. For production, you should add authentication.

---

## Step 7: Test the Connection

### From ESP32:
1. Upload the firmware with your credentials filled in
2. Open **Serial Monitor** at `115200` baud
3. You should see:
   ```
   [WIFI] Connected! IP: 192.168.x.x
   [FIREBASE] ✅ Connected & Authenticated!
   [FIREBASE] ✅ All data pushed successfully!
   ```
4. Go to Firebase Console → Realtime Database → you should see live values updating!

### If it fails:
| Error | Fix |
|-------|-----|
| `WIFI Connection FAILED` | Double-check SSID and password (case-sensitive) |
| `Authentication failed` | Verify API key — copy it again from Project Settings |
| `Failed to push` | Check Database URL — must end with `/` |
| `connection refused` | Make sure Realtime Database is created (not Firestore) |

---

## ✅ Checklist — You Should Now Have:

- [ ] Firebase project created at [console.firebase.google.com](https://console.firebase.google.com)
- [ ] Realtime Database created with initial structure
- [ ] Web app registered and `firebaseConfig` saved
- [ ] ESP32 firmware updated with WiFi + Firebase credentials
- [ ] Database rules published
- [ ] ESP32 successfully pushing data (verify in Firebase Console)

---

## What's Next?

Once you confirm data is flowing from ESP32 → Firebase, we'll build the **React web app** that reads this data and controls the pump! 🚀
