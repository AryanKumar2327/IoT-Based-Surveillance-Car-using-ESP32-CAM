# 🚗 IoT-Based Web Controlled Surveillance Car using ESP32-CAM

<div align="center">

![Status](https://img.shields.io/badge/Status-Completed-brightgreen?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange?style=for-the-badge&logo=cplusplus&logoColor=white)

**A Wi-Fi controlled surveillance vehicle that streams live video to a browser — no app required.**

[Features](#-features) • [Hardware](#-hardware-required) • [Circuit](#-circuit-diagram) • [Installation](#-installation--setup) • [Usage](#-usage) • [Working](#-how-it-works) • [Future Scope](#-future-scope)

</div>

---

## 📌 Project Overview

This project implements a **remotely controlled surveillance car** using the **ESP32-CAM** module. The car can be driven wirelessly over a **local Wi-Fi network** through any standard web browser. A **live MJPEG video stream** from the onboard OV2640 camera is displayed on the web dashboard in real-time, allowing the operator to navigate and monitor the surroundings simultaneously.

The motor drive system is handled by an **L298N H-Bridge motor driver**, which controls two sets of DC motors mounted on a **4-wheel drive car chassis**. The ESP32-CAM itself acts as both the **brain** (processing HTTP requests) and the **eyes** (streaming camera feed) of the vehicle.

> 💡 **No dedicated mobile app needed** — works entirely through a web browser on any device connected to the same Wi-Fi network.

---

## ✨ Features

- 🌐 **Browser-Based Control** — Drive the car from any device with a browser; no installation required
- 📹 **Real-Time Video Streaming** — Live MJPEG stream from the OV2640 2MP camera
- ↔️ **4-Direction Movement** — Forward, Backward, Left, Right via on-screen buttons
- ⚡ **Low-Latency HTTP Commands** — Instant GPIO response for near-real-time motor control
- 🔋 **Fully Portable** — Battery-powered with 18650 Li-ion cells; no wired connection needed
- 📡 **Wi-Fi AP / Station Mode** — Works as a Wi-Fi Access Point or connects to an existing router
- 🔧 **Low-Cost Build** — Total BOM cost under ₹600 / ~$8 USD
- 🛠️ **Open Source & Extensible** — Easy to add sensors, night vision, or AI object detection

---

## 🧰 Hardware Required

| Component | Specification | Quantity |
|-----------|--------------|----------|
| **ESP32-CAM Module** | AI-Thinker, OV2640 camera, 240 MHz | 1 |
| **L298N Motor Driver Module** | Dual H-Bridge, 5V–35V, 2A/channel | 1 |
| **Car Chassis Kit** | 4WD acrylic, includes 4× TT DC motors | 1 |
| **18650 Li-ion Battery** | 3.7V, 2000mAh+ | 2 |
| **18650 Battery Holder** | 2-cell series (7.4V output) | 1 |
| **Arduino Nano** | ATmega328P, used as USB-to-Serial programmer for ESP32-CAM | 1 |
| **Jumper Wires** | Male-to-Male, Male-to-Female | ~20 |

---

## 🔌 Circuit Diagram

```
                        +---------------------------+
                        |       ESP32-CAM           |
                        |                           |
  L298N  ←── GPIO 14 ───┤ IO14  (Motor A - IN1)    |
  L298N  ←── GPIO 15 ───┤ IO15  (Motor A - IN2)    |
  L298N  ←── GPIO 13 ───┤ IO13  (Motor B - IN3)    |
  L298N  ←── GPIO 12 ───┤ IO12  (Motor B - IN4)    |
                        │                           |
  5V     ───────────────┤ 5V    (Power Input)       |
  GND    ───────────────┤ GND                       |
                        |       OV2640 Camera       |
                        |      (Onboard Module)     |
                        +---------------------------+

  L298N Motor Driver:
  ┌─────────────────────────────────────────┐
  │  IN1 ── GPIO14    ENA ── 3.3V (always)  │
  │  IN2 ── GPIO15    ENB ── 3.3V (always)  │
  │  IN3 ── GPIO13                          │
  │  IN4 ── GPIO12                          │
  │  VCC ── Battery+ (7.4V)                 │
  │  GND ── Battery- / ESP32 GND            │
  │  OUT1/OUT2 ── Left Motors               │
  │  OUT3/OUT4 ── Right Motors              │
  └─────────────────────────────────────────┘

  Power:
  Battery (7.4V) ──► L298N VCC
  L298N 5V OUT   ──► ESP32-CAM 5V Pin  (during normal operation)
  Arduino Nano USB ──► ESP32-CAM 5V   (during code upload only)
```

> ⚠️ **Important:** The L298N's onboard 5V regulator can power the ESP32-CAM during normal operation. During upload, the ESP32-CAM is powered through the Arduino Nano via its USB connection.

---

## 📁 Project Structure

```
IoT-Surveillance-Car-ESP32CAM/
│
├── src/
│   └── surveillance_car.ino       # Main Arduino sketch
│
├── docs/
│   ├── circuit_diagram.png        # Wiring diagram
│   ├── project_report.pdf         # Full project documentation
│   └── demo.gif                   # Demo GIF of the car in action
│
├── images/
│   ├── hardware_setup.jpg         # Physical build photo
│   ├── web_dashboard.png          # Web interface screenshot
│   └── components.jpg             # Components photo
│
├── README.md                      # This file
```

---

## ⚙️ Installation & Setup

### Prerequisites

- [Arduino IDE](https://www.arduino.cc/en/software) (v1.8.x or v2.x)
- ESP32 Board Package installed in Arduino IDE
- Arduino Nano (used as USB-to-Serial programmer) connected via USB to your PC

### Step 1 — Install ESP32 Board Support

1. Open Arduino IDE → **File** → **Preferences**
2. Add the following URL to *Additional Boards Manager URLs*:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools** → **Board** → **Boards Manager**
4. Search for `esp32` and install **ESP32 by Espressif Systems**

### Step 2 — Configure Board Settings

| Setting | Value |
|---------|-------|
| Board | `AI Thinker ESP32-CAM` |
| Upload Speed | `115200` |
| Flash Frequency | `80MHz` |
| Flash Mode | `QIO` |
| Partition Scheme | `Huge APP (3MB No OTA)` |
| Port | *(your Arduino Nano COM port)* |

### Step 3 — Update Wi-Fi Credentials

Open `src/surveillance_car.ino` and update the following lines:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";      // Replace with your Wi-Fi name
const char* password = "YOUR_WIFI_PASSWORD";  // Replace with your Wi-Fi password
```

### Step 4 — Flash the ESP32-CAM using Arduino Nano

The **Arduino Nano** acts as a USB-to-Serial bridge to upload code to the ESP32-CAM. Before wiring, you must **remove the ATmega328P chip** from the Arduino Nano (or upload an empty sketch to it) so it doesn't interfere with the serial communication.

#### Wiring — Arduino Nano → ESP32-CAM

| Arduino Nano | ESP32-CAM |
|--------------|-----------|
| GND | GND |
| TX (D1) | U0R (RX) |
| RX (D0) | U0T (TX) |
| 3.3V | 3.3V (power during upload) |
| GND | IO0 / GPIO0 (pull LOW to enter flash mode) |
| RST | GND (disable Nano's onboard MCU) |

> 💡 **Tip:** Connecting the Arduino Nano's **RST pin to GND** disables its onboard ATmega328P, turning it into a pure USB-to-Serial converter — no chip removal needed.

#### Upload Steps

1. **Wire** the Arduino Nano to the ESP32-CAM as shown above
2. **Connect GPIO0 to GND** on the ESP32-CAM (this enables flash / bootloader mode)
3. **Plug** the Arduino Nano into your PC via USB
4. In Arduino IDE, select the correct **COM port** for the Arduino Nano
5. Click **Upload** — wait for "Connecting..." then flashing to begin
6. Once upload shows **Done uploading**, **disconnect GPIO0 from GND**
7. Press the **RST / EN button** on the ESP32-CAM to reboot into normal mode

### Step 5 — Get the IP Address

Open Serial Monitor at **115200 baud**. After boot you will see:

```
WiFi connected!
Camera Stream Ready!
IP Address: 192.168.1.105
```

---

## 🕹️ Usage

1. Connect your phone/laptop to the **same Wi-Fi network** as the ESP32-CAM
2. Open any browser and navigate to the IP address shown in Serial Monitor:
   ```
   http://192.168.1.105
   ```
3. The web dashboard loads with:
   - **Live camera feed** at the top
   - **Direction control buttons** (Forward / Backward / Left / Right / Stop)
4. Tap or click the buttons to drive the car while watching the live stream

### HTTP API Endpoints

You can also control the car programmatically via HTTP GET requests:

| Endpoint | Action |
|----------|--------|
| `/forward` | Move forward |
| `/backward` | Move backward |
| `/left` | Turn left |
| `/right` | Turn right |
| `/stop` | Stop all motors |
| `/stream` | Access live video stream |

---

## ⚙️ How It Works

```
┌──────────┐   HTTP GET    ┌────────────┐   GPIO Signals   ┌──────────┐   PWM   ┌────────┐
│  Browser │ ────────────► │ ESP32-CAM  │ ───────────────► │  L298N   │ ──────► │ Motors │
│  (User)  │              │ Web Server │                   │  Driver  │         │        │
└──────────┘              └─────┬──────┘                   └──────────┘         └────────┘
     ▲                          │
     │    MJPEG Stream          │ OV2640 Camera
     └──────────────────────────┘ (captures frames → encodes → streams)
```

1. **Boot** — ESP32-CAM connects to Wi-Fi and starts an HTTP server on port 80
2. **Dashboard** — Browser loads the HTML control page served by the ESP32-CAM
3. **Command** — User presses a button; browser sends `GET /forward` to the ESP32 IP
4. **Motor Drive** — ESP32-CAM sets GPIO pins HIGH/LOW → L298N drives motors
5. **Stream** — Camera continuously captures MJPEG frames → streamed back to browser via multipart HTTP response
6. **Navigate** — User sees live feed and steers the car in real time

---

## 📊 GPIO Pin Mapping

| GPIO Pin | Function | L298N Pin |
|----------|----------|-----------|
| GPIO 14 | Motor A Direction 1 | IN1 |
| GPIO 15 | Motor A Direction 2 | IN2 |
| GPIO 13 | Motor B Direction 1 | IN3 |
| GPIO 12 | Motor B Direction 2 | IN4 |
| 5V | Power Supply | VCC (logic) |
| GND | Ground | GND |

### Motor Direction Truth Table

| Action | IN1 | IN2 | IN3 | IN4 |
|--------|-----|-----|-----|-----|
| Forward | HIGH | LOW | HIGH | LOW |
| Backward | LOW | HIGH | LOW | HIGH |
| Left | LOW | HIGH | HIGH | LOW |
| Right | HIGH | LOW | LOW | HIGH |
| Stop | LOW | LOW | LOW | LOW |

---

## 📷 Camera Specifications

| Parameter | Value |
|-----------|-------|
| Sensor | OV2640 |
| Resolution | Up to 2MP (1600×1200) |
| Stream Format | MJPEG |
| Recommended Stream Res. | VGA (640×480) for low latency |
| Frame Rate (VGA) | ~15–25 FPS over LAN |
| Interface | DVP (integrated on ESP32-CAM) |

---

## 🐛 Troubleshooting

| Issue | Likely Cause | Solution |
|-------|-------------|----------|
| Upload fails | GPIO0 not pulled to GND | Short GPIO0 → GND on ESP32-CAM before uploading |
| Upload fails | Arduino Nano RST not grounded | Connect Nano RST → GND to disable onboard MCU |
| "Connecting…" loops | Wrong TX/RX wiring | Verify Nano TX → ESP32 RX and Nano RX → ESP32 TX |
| Camera init failed | Insufficient power | Use dedicated 5V source; avoid L298N 5V pin under load |
| IP not showing | Wi-Fi credentials wrong | Double-check SSID and password in code |
| Stream laggy | High resolution set | Change `framesize` to `FRAMESIZE_VGA` or lower |
| Motors not moving | L298N wiring error | Verify IN1–IN4 connections match GPIO pin mapping |
| Car veering off course | Motor polarity mismatch | Swap OUT1/OUT2 wires on one side |
| Browser can't connect | Different subnet | Ensure device and ESP32-CAM are on same Wi-Fi network |

---

## 🔮 Future Scope

- [ ] 🔦 **Night Vision** — Add IR LEDs for low-light surveillance
- [ ] 📡 **Ultrasonic Obstacle Avoidance** — Auto-stop before collisions
- [ ] 🤖 **AI Object Detection** — Run TensorFlow Lite on-device for person/object detection
- [ ] 🌍 **Internet Access (Cloud)** — Control the car from anywhere using MQTT / WebSocket tunneling
- [ ] 📱 **Mobile App** — Flutter/React Native app with joystick control
- [ ] 🎥 **Pan-Tilt Camera Mount** — 180° horizontal + vertical camera rotation via servos
- [ ] 💾 **Video Recording** — Save clips to microSD card on the ESP32-CAM
- [ ] 🔋 **Battery Level Indicator** — ADC-based battery monitoring displayed on dashboard
- [ ] 🛰️ **GPS Tracking** — Add NEO-6M GPS module for location logging

---

## 🙋‍♂️ Author

**Aryan Kumar**
- LinkedIn: [Aryan Kumar](www.linkedin.com/in/aryan-kumar-555556318)

---

<div align="center">

⭐ **If this project helped you, please give it a star!** ⭐

Made with ❤️ using ESP32-CAM · L298N · Car Chassis · Arduino Nano

</div>
