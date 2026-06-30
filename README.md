# Smart IoT-Based Home Security & Environmental Monitoring System

> A cloud-connected embedded security platform integrating multi-sensor intrusion detection, environmental hazard monitoring, and remote actuation on a single ESP32 microcontroller — simulated end-to-end in Wokwi and orchestrated through Blynk Cloud with bidirectional mobile and web-based control.

**Author:** Anik Adnan
**Course:** IoT and Robotics
**Platform:** ESP32 · Wokwi Simulation · Blynk Cloud · Blynk Mobile App

---

## Table of Contents

1. [Abstract](#abstract)
2. [System Architecture](#system-architecture)
3. [Hardware Components](#hardware-components)
4. [Software and Libraries](#software-and-libraries)
5. [Pin Configuration and Wiring](#pin-configuration-and-wiring)
6. [Blynk Cloud Configuration](#blynk-cloud-configuration)
7. [Trigger Conditions and System Responses](#trigger-conditions-and-system-responses)
8. [Alarm Priority and Latch Behaviour](#alarm-priority-and-latch-behaviour)
9. [Installation and Setup](#installation-and-setup)
10. [Repository Structure](#repository-structure)
11. [Testing and Validation](#testing-and-validation)
12. [Results](#results)
13. [Limitations and Future Work](#limitations-and-future-work)
14. [License](#license)
15. [Acknowledgements](#acknowledgements)

---

## Abstract

This project presents the design and simulation of a Smart IoT-Based Home Security and Environmental Monitoring System constructed on the ESP32 microcontroller. The system consolidates three functional subsystems — intrusion detection via ultrasonic proximity sensing, environmental hazard monitoring via temperature and humidity acquisition, and remote access control via servo-driven door actuation — into a unified embedded platform. Real-time telemetry and bidirectional control are delivered through Blynk Cloud and its companion mobile application. All development and functional validation were conducted within the Wokwi browser-based circuit simulator, satisfying the assignment's simulation requirements. The implementation demonstrates a complete, end-to-end IoT pipeline spanning sensor acquisition, edge-level threshold logic, cloud synchronisation, and human-in-the-loop mobile control, across eight interconnected hardware components.

---

## System Architecture

The system is organised into four hierarchical layers:

```
[ Perception Layer ]  →  [ Processing Layer ]  →  [ Connectivity Layer ]  →  [ Application Layer ]
  Sensors / Actuators      ESP32 Edge Logic           WiFi · Blynk Cloud        Web · Mobile App
```

| Layer | Components | Responsibility |
|---|---|---|
| **Perception** | HC-SR04, DHT22, LDR | Environmental and proximity data acquisition |
| **Processing** | ESP32 DevKit V1 | Threshold evaluation, actuator control, state management |
| **Connectivity** | WiFi (Wokwi-GUEST), Blynk Cloud | Telemetry transport and event delivery |
| **Application** | Blynk Web Dashboard, Blynk Mobile App | Remote monitoring, manual override, alarm reset |

---

## Hardware Components

| Component | Qty | Role |
|---|---|---|
| ESP32 DevKit V1 | 1 | Central microcontroller |
| HC-SR04 Ultrasonic Sensor | 1 | Intrusion and proximity detection |
| DHT22 Temperature & Humidity Sensor | 1 | Environmental monitoring and fire-risk detection |
| LDR (Photoresistor) | 1 | Ambient light sensing for auto-lighting and night-mode indication |
| Servo Motor (SG90) | 1 | Electronic door lock actuation |
| Active Buzzer | 1 | Audible siren alarm output |
| RGB LED (Common Cathode) | 1 | Visual system status indicator |
| Relay Module | 1 | Switching control for room light or appliance |
| OLED Display — SSD1306, I2C, 128×64 | 1 | Local on-device live status dashboard |
| Resistors — 220 Ω ×3, 10 kΩ ×1 | 4 | Current limiting (RGB LED) and voltage divider (LDR) |

---

## Software and Libraries

| Library | Purpose |
|---|---|
| `Blynk` | Cloud connectivity and virtual pin communication |
| `DHT sensor library` (Adafruit) | DHT22 temperature and humidity acquisition |
| `Adafruit Unified Sensor` | Hardware abstraction dependency for DHT library |
| `Adafruit GFX Library` | Graphics primitives for OLED rendering |
| `Adafruit SSD1306` | OLED I2C display driver |
| `ESP32Servo` | PWM-based servo motor control on ESP32 |

All simulation and firmware development were conducted within [Wokwi](https://wokwi.com), an in-browser embedded systems simulator supporting ESP32, component wiring, and library management.

---

## Pin Configuration and Wiring

| Component | VCC | GND | Signal Pin(s) | Notes |
|---|---|---|---|---|
| HC-SR04 | 5V | GND | TRIG → GPIO 5, ECHO → GPIO 18 | ECHO is input-only |
| DHT22 | 3V3 | GND | GPIO 4 | Built-in pull-up in Wokwi |
| LDR | 3V3 | GND via 10 kΩ | GPIO 34 (ADC1) | Voltage-divider; GPIO 34 is input-only |
| Servo Motor | 5V | GND | GPIO 13 | Orange wire = PWM signal |
| Active Buzzer | 5V | GND | GPIO 25 | Driven by `tone()` / `noTone()` |
| RGB LED | — | GND (cathode) | R → 26, G → 27, B → 14 | 220 Ω resistor per colour leg |
| Relay Module | 5V | GND | GPIO 32 | Active-HIGH input |
| OLED SSD1306 | 3V3 | GND | SDA → 21, SCL → 22 | I2C address 0x3C |

> **Buzzer note:** `digitalWrite(HIGH)` produces only a DC signal and generates no audible output on a piezo buzzer. This implementation uses `tone(BUZZER_PIN, frequency)` to generate a hardware square wave and `noTone(BUZZER_PIN)` to stop it, ensuring reliable audio output in both simulation and physical hardware.

---

## Blynk Cloud Configuration

### Datastreams — Virtual Pins

| Pin | Datastream | Data Type | Direction | Description |
|---|---|---|---|---|
| V0 | Temperature | Double (°C) | Device → Cloud | Live DHT22 temperature reading |
| V1 | Humidity | Double (%) | Device → Cloud | Live DHT22 humidity reading |
| V2 | Distance | Double (cm) | Device → Cloud | Live HC-SR04 proximity reading |
| V3 | Light Level | Integer | Device → Cloud | Live LDR ADC reading |
| V4 | Door Lock | Integer (Switch) | Bidirectional | Lock/unlock control; forced LOCKED during active alarm |
| V5 | Room Light | Integer (Switch) | App → Device | Manual relay override |
| V7 | Alarm Reset | Integer (Button) | App → Device | Clears alarm state and unlocks door |

### Event

| Event Code | Trigger Condition | Delivery |
|---|---|---|
| `security_alert` | Intrusion or fire-risk threshold exceeded | Push notification to Blynk mobile app |

---

## Trigger Conditions and System Responses

The system evaluates four distinct operating states on every sensor read cycle (every 2 seconds). The buzzer siren alternates between **800 Hz** and **1400 Hz** every 400 ms using non-blocking `millis()` timing, producing a continuous two-tone alarm without interrupting the main control loop.

---

### Condition 1 — Intrusion Detected

**Trigger:** HC-SR04 distance reading falls below the intrusion threshold.

> Demo threshold: `400 cm` (Wokwi — drag HC-SR04 slider below this value)
> Deployment threshold: `15 cm` (physical hardware)

| Component | Response |
|---|---|
| **Buzzer** | Siren activates — alternates 800 Hz ↔ 1400 Hz every 400 ms continuously |
| **RGB LED** | → **RED** |
| **Servo (Door)** | Rotates to 0° → **LOCKED** immediately |
| **Relay (Room Light)** | Unchanged — continues under LDR auto-mode or V5 manual override |
| **OLED** | Displays `!! ALERT !!` with live distance value |
| **Blynk Cloud** | V4 forced to `1` (locked); V2 updates live; `security_alert` push notification fired |
| **Serial Monitor** | Prints distance value followed by `<<< INTRUSION!` |

---

### Condition 2 — Fire / Heat Risk Detected

**Trigger:** DHT22 temperature reading exceeds 40 °C.

| Component | Response |
|---|---|
| **Buzzer** | Siren activates — same 800 Hz ↔ 1400 Hz pattern as intrusion |
| **RGB LED** | → **RED** |
| **Servo (Door)** | Rotates to 0° → **LOCKED** |
| **Relay (Room Light)** | Unchanged — alarm does not interfere with lighting logic |
| **OLED** | Displays `!! ALERT !!` with live temperature value |
| **Blynk Cloud** | V4 forced to `1`; V0 updates live; `security_alert` push notification fired |
| **Serial Monitor** | Prints temperature value followed by `<<< FIRE RISK!` |

---

### Condition 3 — Night Mode (No Alarm Active)

**Trigger:** LDR ADC reading falls below 1500 (low ambient light) and no alarm is currently active.

| Component | Response |
|---|---|
| **Buzzer** | **Silent** — `noTone()` enforced |
| **RGB LED** | → **BLUE** |
| **Servo (Door)** | Unchanged — retains last locked or unlocked state |
| **Relay (Room Light)** | → **ON** automatically (unless V5 manual override is active) |
| **OLED** | Displays `SAFE` with live light level |
| **Blynk Cloud** | V3 reflects low light reading; V5 relay state updated |

---

### Condition 4 — All Clear / Daytime (No Alarm Active)

**Trigger:** Distance ≥ threshold AND temperature ≤ 40 °C AND LDR ADC ≥ 1500 AND no alarm active.

| Component | Response |
|---|---|
| **Buzzer** | **Silent** |
| **RGB LED** | → **GREEN** |
| **Servo (Door)** | Unchanged — retains last locked or unlocked state |
| **Relay (Room Light)** | → **OFF** automatically (LDR reads sufficient ambient light) |
| **OLED** | Displays `SAFE` across all fields |
| **Blynk Cloud** | All datastreams V0–V3 update normally every 2 seconds |

---

### Condition 5 — Alarm Reset (V7 Button)

**Trigger:** User taps the Alarm Reset button (V7) in the Blynk mobile app or web console.

| Component | Response |
|---|---|
| **Buzzer** | `noTone()` called → **SILENT** immediately |
| **RGB LED** | → **GREEN** (or **BLUE** if LDR still reads dark) |
| **Servo (Door)** | Rotates to 90° → **UNLOCKED** |
| **Relay (Room Light)** | Returns to LDR auto-mode (unless V5 override remains ON) |
| **OLED** | Displays `SAFE` |
| **Blynk Cloud** | V4 resets to `0` (unlocked) |

---

## Alarm Priority and Latch Behaviour

```
ALARM (Conditions 1 or 2)  >  Manual App Overrides  >  Night Mode  >  All Clear
```

| Rule | Detail |
|---|---|
| **Alarm latches** | Once triggered, the alarm persists regardless of sensor recovery until V7 is explicitly pressed |
| **Door lock is protected** | V4 override from the app is rejected while an alarm is active; the UI is forced back to locked |
| **Relay is independent** | The room light (relay) is the only component the alarm logic does not modify; it operates under LDR auto-mode or V5 at all times |
| **Dual trigger** | If both intrusion and fire-risk conditions are simultaneously true, the intrusion branch fires first (evaluated first in code); both produce identical system responses |
| **Siren is non-blocking** | `runSiren()` is called on every `loop()` tick via `millis()` comparison, not `delay()`, ensuring Blynk connectivity and sensor reads are never interrupted |

---

## Installation and Setup

### 1 — Wokwi Simulation

1. Create a free account at [wokwi.com](https://wokwi.com).
2. Create a new ESP32 project and import `diagram.json` from this repository, or wire components manually per the [Pin Configuration](#pin-configuration-and-wiring) table.
3. Paste the contents of `Smart_IoT_Security_System.ino` into the sketch editor.
4. Add required libraries via the Wokwi Library Manager (see [Software and Libraries](#software-and-libraries)).

### 2 — Blynk Cloud

1. Register at [blynk.cloud](https://blynk.cloud).
2. Create a new Template: name `Smart IoT Security System`, hardware `ESP32`, connection `WiFi`.
3. Add all datastreams and the `security_alert` event per the [Blynk Cloud Configuration](#blynk-cloud-configuration) tables.
4. Create a Device from the template; copy the **Auth Token**, **Template ID**, and **Template Name**.
5. Insert these three credentials into the top of `Smart_IoT_Security_System.ino`.
6. Build a Web Dashboard: gauge widgets for V0–V3; switch widgets for V4, V5; button widget for V7.

### 3 — Blynk Mobile App

1. Install **Blynk IoT** from the App Store or Google Play.
2. Log in with the same Blynk Cloud credentials — the device appears automatically.
3. Open the device, tap the edit (pencil) icon, and add matching widgets for V0–V7.
4. Save the layout; widgets activate immediately against the live simulation.

### 4 — Run and Test

Click **▶ Play** in Wokwi to compile and launch the simulation. To observe each alarm condition:

| Test | Action in Wokwi |
|---|---|
| Intrusion | Click HC-SR04 → drag distance slider below 400 cm |
| Fire risk | Click DHT22 → raise temperature slider above 40 °C |
| Night mode | Click LDR → lower light slider below 1500 ADC |
| Alarm reset | Tap V7 button in Blynk app or web dashboard |

---

## Repository Structure

```
.
├── Smart_IoT_Security_System.ino   # Main ESP32 firmware
├── diagram.json                    # Wokwi circuit definition
├── libraries.txt                   # Wokwi library manifest
├── Screenshots/
│   ├── Blynk-Console-Dashboard.png
│   ├── Blynk-Console-Datastream.png
│   ├── Blynk-Console-Events.png
│   ├── Blynk-Console-Home.png
│   ├── Blynk-IoT-Control-Widgets.jpeg
│   ├── Blynk-Mobile-Dashboard-Successful-Operation-through-Connected-Devices.png
│   ├── OLED-Display-Monitor-Output.png
│   ├── Wowki-Circuit-Diagram.png
│   ├── Wowki-Diagram-with-Output.png
│   └── Wowki-Profile.png
├── report/
│   └── Project_Report.pdf
└── README.md
```

---

## Testing and Validation

| Test Case | Trigger | Expected Outcome |
|---|---|---|
| Intrusion detection | HC-SR04 < 400 cm | Siren (800/1400 Hz), RGB → RED, servo locks, push alert |
| Fire-risk detection | DHT22 > 40 °C | Same alarm sequence as intrusion |
| Night-mode indication | LDR ADC < 1500 | RGB → BLUE, relay ON, buzzer silent |
| Daytime / all-clear | All sensors within range | RGB → GREEN, relay OFF, buzzer silent |
| Remote lock override | Toggle V4 (no alarm) | Servo responds; ignored if alarm is active |
| Alarm reset | Tap V7 in app | Buzzer stops, door unlocks, RGB → GREEN |
| Cloud synchronisation | Simulation running | V0–V3 update on Blynk dashboard every 2 seconds |

---

## Results

The system successfully demonstrates autonomous threshold-based decision-making at the edge (ESP32), delivering sub-second actuator responses upon threshold breach, combined with cloud-mediated remote monitoring and override via Blynk. All five operating conditions — intrusion, fire-risk, night mode, all-clear, and alarm reset — were validated within the Wokwi simulation environment. The implementation satisfies the assignment requirement of a minimum three input/output devices, realised here across eight interconnected components, and establishes a complete IoT pipeline from sensor acquisition through cloud synchronisation to mobile-based control.

---

## Limitations and Future Work

- The current implementation is validated exclusively in simulation (Wokwi); deployment on physical hardware would require recalibration of sensor thresholds and compensation for real-world noise, reflective surfaces, and sensor tolerance.
- The intrusion distance threshold is set to 400 cm for demonstration purposes within the Wokwi HC-SR04 slider range; it should be reduced to 15 cm for physical deployment.
- Authentication for the door-lock override is scoped to the Blynk account level. A production implementation would incorporate PIN-based or biometric confirmation within the application layer.
- Sensor fusion (combining ultrasonic proximity with LDR light change and PIR motion data) is identified as a direction for improving intrusion detection specificity and reducing false-positive events.
- Integration of an edge machine learning model for anomaly detection, leveraging the ESP32's dual-core architecture, represents a viable extension for distinguishing between occupant movement and genuine intrusion events.

---

## Acknowledgements

Developed as part of the *IoT and Robotics* coursework using the [Wokwi](https://wokwi.com) embedded systems simulator and [Blynk IoT Cloud](https://blynk.cloud) platform.
