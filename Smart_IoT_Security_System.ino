/* ============================================================
   PROJECT      : Smart IoT-Based Home Security & Environmental
                  Monitoring System with Cloud-Based Real-Time Alerting
   COURSE       : IoT and Robotics
   STUDENT NAME : Anik Adnan
   STUDENT ID   : <<REPLACE_WITH_YOUR_STUDENT_ID>>
   PLATFORM     : ESP32 + Wokwi Simulation + Blynk Cloud
   ============================================================
   DESCRIPTION:
   This system fuses intrusion detection (ultrasonic sensor),
   environmental monitoring (DHT22), ambient light sensing (LDR),
   and remote actuation (servo door lock + relay-controlled light)
   into a single cloud-connected security/monitoring platform.
   All data is streamed live to Blynk Cloud and the Blynk mobile
   app, with local status shown on an OLED display.
   ============================================================ */

// ---------- BLYNK CREDENTIALS (replace with your own from Blynk.Cloud) ----------
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxx"
#define BLYNK_TEMPLATE_NAME "Smart IoT Security System"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- WIFI CREDENTIALS (Wokwi virtual network) ----------
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// ---------- PIN DEFINITIONS ----------
#define TRIG_PIN      5     // HC-SR04 Ultrasonic - Trigger
#define ECHO_PIN      18    // HC-SR04 Ultrasonic - Echo
#define DHT_PIN       4     // DHT22 Data pin
#define LDR_PIN       34    // LDR analog input (ADC1)
#define SERVO_PIN     13    // Servo - door lock actuator
#define BUZZER_PIN    25    // Buzzer - alarm
#define RED_PIN       26    // RGB LED - Red
#define GREEN_PIN     27    // RGB LED - Green
#define BLUE_PIN      14    // RGB LED - Blue
#define RELAY_PIN     32    // Relay module - room light/fan

#define DHT_TYPE      DHT22
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// ---------- THRESHOLDS ----------
const float  INTRUSION_DISTANCE_CM = 15.0;   // distance under which entry is "breached"
const float  FIRE_TEMP_THRESHOLD   = 40.0;   // deg C - fire/heat risk
const int    DARK_THRESHOLD        = 1500;   // LDR analog value below = "dark"

// ---------- OBJECTS ----------
DHT dht(DHT_PIN, DHT_TYPE);
Servo doorLockServo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- STATE VARIABLES ----------
bool doorLocked      = true;   // current lock state (true = locked)
bool manualLightOn   = false;  // manual override flag for relay
bool alarmActive     = false;  // intrusion/fire alarm state
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 2000; // read sensors every 2s

// ============================================================
//  HELPER FUNCTIONS
// ============================================================

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  if (duration == 0) return 999.0; // no echo = treat as "clear"
  return duration * 0.0343 / 2.0;  // convert to cm
}

void setRGB(bool red, bool green, bool blue) {
  digitalWrite(RED_PIN, red);
  digitalWrite(GREEN_PIN, green);
  digitalWrite(BLUE_PIN, blue);
}

void lockDoor() {
  doorLockServo.write(0);   // 0 degrees = locked position
  doorLocked = true;
  Blynk.virtualWrite(V4, 1);
  Serial.println("Door Lock: LOCKED");
}

void unlockDoor() {
  doorLockServo.write(90);  // 90 degrees = unlocked position
  doorLocked = false;
  Blynk.virtualWrite(V4, 0);
  Serial.println("Door Lock: UNLOCKED");
}

void triggerAlarm(String reason) {
  if (!alarmActive) {
    alarmActive = true;
    Serial.println("*** ALERT TRIGGERED: " + reason + " ***");
    Blynk.logEvent("security_alert", reason); // requires "security_alert" event set up in Blynk console
  }
  digitalWrite(BUZZER_PIN, HIGH);
  setRGB(true, false, false); // RED
  lockDoor();                 // security overrides manual state
}

void clearAlarm() {
  alarmActive = false;
  digitalWrite(BUZZER_PIN, LOW);
  setRGB(false, true, false); // GREEN = safe
  Serial.println("Alarm cleared. System status: SAFE");
}

// ============================================================
//  BLYNK APP -> DEVICE CONTROL HANDLERS
// ============================================================

// V4: Door lock switch (manual override) - ignored while alarm is active
BLYNK_WRITE(V4) {
  int value = param.asInt();
  if (!alarmActive) {
    if (value == 1) lockDoor();
    else unlockDoor();
  } else {
    Blynk.virtualWrite(V4, 1); // force UI back to locked during alarm
  }
}

// V5: Room light / relay manual switch
BLYNK_WRITE(V5) {
  manualLightOn = param.asInt();
  digitalWrite(RELAY_PIN, manualLightOn ? HIGH : LOW);
  Serial.println(manualLightOn ? "Room Light: MANUAL ON" : "Room Light: MANUAL OFF");
}

// V7: Alarm reset button from the app
BLYNK_WRITE(V7) {
  if (param.asInt() == 1) {
    clearAlarm();
    unlockDoor();
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("============================================");
  Serial.println("Student Name : Anik Adnan");
  Serial.println("Student ID   : <<REPLACE_WITH_YOUR_STUDENT_ID>>");
  Serial.println("Project Title: Smart IoT Security & Environmental Monitoring System");
  Serial.println("============================================");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);

  dht.begin();
  doorLockServo.attach(SERVO_PIN);
  lockDoor(); // start in locked/safe state
  setRGB(false, true, false); // start GREEN (safe)
  digitalWrite(RELAY_PIN, LOW);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart IoT Security");
  display.println("System Booting...");
  display.display();

  Blynk.begin(auth, ssid, pass);
  Serial.println("ESP32 connected. Waiting for Blynk commands...");
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  Blynk.run();

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();

    // ---- Read sensors ----
    float distance   = readDistanceCM();
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();
    int   lightLevel  = analogRead(LDR_PIN);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("DHT22 read failed!");
      temperature = 0; humidity = 0;
    }

    // ---- Push to Blynk Cloud ----
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, distance);
    Blynk.virtualWrite(V3, lightLevel);

    // ---- Auto light control via LDR (only if no manual override) ----
    if (!manualLightOn) {
      digitalWrite(RELAY_PIN, (lightLevel < DARK_THRESHOLD) ? HIGH : LOW);
    }

    // ---- Security & safety logic ----
    if (distance < INTRUSION_DISTANCE_CM) {
      triggerAlarm("Intrusion detected at entry point: " + String(distance) + " cm");
    } else if (temperature > FIRE_TEMP_THRESHOLD) {
      triggerAlarm("High temperature risk: " + String(temperature) + " C");
    } else if (!alarmActive) {
      // No active alarm: show GREEN (daylight, all clear) or BLUE (night, all clear)
      if (lightLevel < DARK_THRESHOLD) {
        setRGB(false, false, true);  // BLUE = night mode, safe
      } else {
        setRGB(false, true, false);  // GREEN = daylight, safe
      }
    }

    // ---- Serial Monitor output ----
    Serial.println("---------------------------------------------");
    Serial.println("Student Name : Anik Adnan | ID: <<REPLACE_WITH_YOUR_STUDENT_ID>>");
    Serial.println("Project      : Smart IoT Security & Environmental Monitoring System");
    Serial.print("Distance: "); Serial.print(distance); Serial.println(" cm");
    Serial.print("Temp: "); Serial.print(temperature); Serial.print(" C | Humidity: ");
    Serial.print(humidity); Serial.println(" %");
    Serial.print("Light Level (LDR): "); Serial.println(lightLevel);
    Serial.print("Door Lock: "); Serial.println(doorLocked ? "LOCKED" : "UNLOCKED");
    Serial.print("Alarm Status: "); Serial.println(alarmActive ? "ACTIVE" : "SAFE");

    // ---- OLED local dashboard ----
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Smart IoT Security Sys");
    display.print("Temp: "); display.print(temperature); display.println(" C");
    display.print("Hum : "); display.print(humidity); display.println(" %");
    display.print("Dist: "); display.print(distance); display.println(" cm");
    display.print("Lock: "); display.println(doorLocked ? "LOCKED" : "UNLOCKED");
    display.print("Stat: "); display.println(alarmActive ? "!! ALERT !!" : "SAFE");
    display.display();
  }
}
