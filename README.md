# Smart IoT-Based Home Security & Environmental Monitoring System

> A cloud-connected embedded system integrating intrusion detection, environmental safety monitoring, and remote actuation on a single ESP32 microcontroller, simulated in Wokwi and orchestrated through Blynk Cloud.

**Author:** Anik Adnan
**Course:** IoT and Robotics
**Platform:** ESP32 · Wokwi Simulation · Blynk Cloud · Blynk Mobile App

---

## Table of Contents

1. [Abstract](#abstract)
2. [System Architecture](#system-architecture)
3. [Hardware Components](#hardware-components)
4. [Software & Libraries](#software--libraries)
5. [Wiring Diagram](#wiring-diagram)
6. [Blynk Cloud Configuration](#blynk-cloud-configuration)
7. [Trigger Logic & Automation Rules](#trigger-logic--automation-rules)
8. [Installation & Setup](#installation--setup)
9. [Repository Structure](#repository-structure)
10. [Usage](#usage)
11. [Testing & Validation](#testing--validation)
12. [Results](#results)
13. [Limitations & Future Work](#limitations--future-work)
14. [License](#license)
15. [Acknowledgements](#acknowledgements)

---

## Abstract

This project presents the design and simulation of a Smart IoT-Based Home Security and Environmental Monitoring System built on the ESP32 microcontroller. The system unifies three functional subsystems — intrusion detection, environmental hazard monitoring, and remote access control — into a single embedded platform, with real-time telemetry and bidirectional control delivered through Blynk Cloud and its companion mobile application. The implementation was developed and validated entirely within the Wokwi browser-based simulator, in accordance with the assignment's circuit-simulation requirements, and demonstrates an end-to-end IoT pipeline from sensor acquisition through cloud synchronisation to mobile-based human-in-the-loop control.

## System Architecture

The system follows a four-layer architecture:

```
[ Sensors / Actuators ] → [ ESP32 + WiFi ] → [ Blynk Cloud ] → [ Mobile / Web App ]
```

| Layer | Role |
|---|---|
| **Perception Layer** | HC-SR04, DHT22, and LDR acquire environmental and proximity data |
| **Processing Layer** | ESP32 executes threshold-based decision logic and drives actuators |
| **Connectivity Layer** | WiFi (Wokwi-GUEST in simulation) transports telemetry to Blynk Cloud |
| **Application Layer** | Blynk Web Dashboard and Mobile App provide monitoring and remote override |

## Hardware Components

| Component | Quantity | Function |
|---|---|---|
| ESP32 DevKit V1 | 1 | Central microcontroller |
| HC-SR04 Ultrasonic Sensor | 1 | Intrusion / proximity detection |
| DHT22 Sensor | 1 | Temperature and humidity monitoring |
| LDR (Photoresistor) | 1 | Ambient light sensing |
| Servo Motor (SG90) | 1 | Simulated electronic door lock |
| Active Buzzer | 1 | Audible alarm |
| RGB LED | 1 | Visual system status indicator |
| Relay Module | 1 | Switching for room light / appliance |
| OLED Display (SSD1306, I2C) | 1 | Local on-device status dashboard |
| Resistors (220 Ω ×3, 10 kΩ ×1) | 4 | Current limiting / voltage divider |

## Software & Libraries

| Library | Purpose |
|---|---|
| `Blynk` | Cloud connectivity and virtual pin communication |
| `DHT sensor library` (Adafruit) | DHT22 temperature/humidity readout |
| `Adafruit Unified Sensor` | Dependency for the DHT library |
| `Adafruit GFX Library` | Graphics primitives for the OLED |
| `Adafruit SSD1306` | OLED display driver |
| `ESP32Servo` | PWM servo control on ESP32 |

Development and simulation were carried out entirely within [Wokwi](https://wokwi.com), an in-browser circuit and firmware simulator.

## Wiring Diagram

| Component | VCC | GND | Signal |
|---|---|---|---|
| HC-SR04 | 5V | GND | TRIG → GPIO 5, ECHO → GPIO 18 |
| DHT22 | 3V3 | GND | GPIO 4 |
| LDR | 3V3 | GND (via 10 kΩ) | GPIO 34 (ADC1) |
| Servo Motor | 5V | GND | GPIO 13 |
| Buzzer | 5V | GND | GPIO 25 |
| RGB LED | — | GND (common cathode) | R → GPIO 26, G → GPIO 27, B → GPIO 14 |
| Relay Module | 5V | GND | GPIO 32 |
| OLED SSD1306 | 3V3 | GND | SDA → GPIO 21, SCL → GPIO 22 |

> A full annotated wiring diagram (`/docs/wiring-diagram.png`) and the Wokwi `diagram.json` circuit definition are included in this repository.

## Blynk Cloud Configuration

### Datastreams (Virtual Pins)

| Pin | Datastream | Type | Description |
|---|---|---|---|
| V0 | Temperature | Double (°C) | Live DHT22 reading |
| V1 | Humidity | Double (%) | Live DHT22 reading |
| V2 | Distance | Double (cm) | Live HC-SR04 reading |
| V3 | Light Level | Integer | Live LDR reading |
| V4 | Door Lock | Switch | Bidirectional lock control |
| V5 | Room Light | Switch | Manual relay override |
| V7 | Alarm Reset | Button | Clears active alarm state |

### Event

| Event Code | Trigger |
|---|---|
| `security_alert` | Fired on intrusion or high-temperature detection; delivers a push notification |

## Trigger Logic & Automation Rules

| Condition | Threshold | System Response |
|---|---|---|
| Intrusion detected | Distance < 15 cm | Buzzer ON, RGB → RED, door auto-locks, push alert |
| Fire / heat risk | Temperature > 40 °C | Buzzer ON, RGB → RED, door auto-locks, push alert |
| Night, no alarm | Light level < 1500 | RGB → BLUE, relay auto-ON (if no manual override) |
| Daylight, no alarm | Light level ≥ 1500 | RGB → GREEN |
| Alarm acknowledged | V7 button pressed | Alarm cleared, buzzer OFF, door unlocked |

The alarm state is **latching**: once triggered, it persists until explicitly cleared via the Alarm Reset control, ensuring transient sensor fluctuations cannot silently dismiss a genuine security event.

## Installation & Setup

### 1. Wokwi Simulation

1. Create a free account at [wokwi.com](https://wokwi.com).
2. Create a new ESP32 project and import `diagram.json` from this repository, or manually wire components per the [Wiring Diagram](#wiring-diagram).
3. Paste the contents of `Smart_IoT_Security_System.ino` into the sketch editor.
4. Add the required libraries (see [Software & Libraries](#software--libraries)) via the Wokwi Library Manager.

### 2. Blynk Cloud

1. Register at [blynk.cloud](https://blynk.cloud).
2. Create a new Template (`Smart IoT Security System`, Hardware: ESP32, Connection: WiFi).
3. Add the datastreams and event listed in [Blynk Cloud Configuration](#blynk-cloud-configuration).
4. Create a Device from the template and copy its **Auth Token**, along with the **Template ID** and **Template Name**.
5. Insert these three credentials into the top of `Smart_IoT_Security_System.ino`.
6. Build a Web Dashboard with gauge widgets for V0–V3 and switch/button widgets for V4, V5, V7.

### 3. Blynk Mobile App

1. Install **Blynk IoT** from the App Store or Google Play.
2. Log in using the same Blynk Cloud account.
3. Open the auto-listed device and add matching widgets in edit mode.
4. Save the layout to enable live monitoring and control.

### 4. Run

Click **▶ Play** in Wokwi to compile and start the simulation. The Blynk dashboard and mobile app should reflect live sensor data within a few seconds, with the device status shown as **Online**.

## Repository Structure

```
.
├── Smart_IoT_Security_System.ino   # Main firmware source
├── diagram.json                    # Wokwi circuit definition
├── libraries.txt                   # Wokwi library manifest
├── docs/
│   ├── wiring-diagram.png
│   ├── blynk-console-screenshot.png
│   ├── blynk-mobile-screenshot.png
│   └── system-architecture.png
├── report/
│   └── Project_Report.pdf
└── README.md
```

## Usage

A demonstration video covering circuit assembly, simulation, Blynk Cloud configuration, and mobile app control is available at: `<insert demo video link / QR code>`

## Testing & Validation

| Test Case | Procedure | Expected Outcome | Result |
|---|---|---|---|
| Intrusion detection | Reduce HC-SR04 distance below 15 cm | Buzzer activates, RGB turns red, door locks, push alert received | ✅ |
| Fire-risk detection | Raise DHT22 temperature above 40 °C | Same alarm sequence as intrusion | ✅ |
| Night-mode indication | Reduce LDR light level below threshold | RGB turns blue, relay activates | ✅ |
| Remote lock override | Toggle V4 from mobile app | Servo position changes accordingly | ✅ |
| Alarm reset | Tap V7 in mobile app during active alarm | Alarm clears, door unlocks | ✅ |
| Cloud synchronisation | Observe Blynk Web Dashboard during simulation | Live values update every 2 seconds | ✅ |

## Results

The system successfully demonstrates autonomous threshold-based decision-making at the edge (ESP32) combined with cloud-mediated remote monitoring and override, achieving the assignment's objective of a multi-sensor, cloud-integrated embedded system with a minimum of three input/output devices — here implemented with eight.

## Limitations & Future Work

- The current implementation is validated in simulation (Wokwi); deployment on physical hardware would require recalibration of sensor thresholds for real-world noise conditions.
- Authentication for the door-lock override is currently scoped to the Blynk account level; a future iteration could add PIN-based confirmation within the app.
- Edge-based anomaly detection (e.g., distinguishing a pet from an intruder via sensor fusion) is identified as a direction for future extension.

## License

This project is released under the MIT License. See `LICENSE` for details.

## Acknowledgements

Developed as part of the *IoT and Robotics* coursework, using the Wokwi simulation platform and Blynk IoT Cloud.
