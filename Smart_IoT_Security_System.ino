/* ============================================================
   PROJECT      : Smart IoT-Based Home Security & Environmental
                  Monitoring System with Cloud-Based Real-Time Alerting
   COURSE       : IoT and Robotics
   STUDENT NAME : Anik Adnan
   STUDENT ID   : <<REPLACE_WITH_YOUR_STUDENT_ID>>
   PLATFORM     : ESP32 + Wokwi Simulation + Blynk Cloud

   ALARM TRIGGER CONDITIONS:
   ┌─────────────────────────────────────────────────────────────┐
   │  INTRUSION   Distance < 400 cm  → Siren + RED LED + Lock   │
   │  FIRE RISK   Temp     > 40  °C  → Siren + RED LED + Lock   │
   │  NIGHT MODE  LDR      < 1500    → Relay ON + BLUE LED      │
   │  ALL CLEAR   No alarm active    → GREEN LED (silent)        │
   │  ALARM RESET V7 button (Blynk)  → Clear + Unlock           │
   └─────────────────────────────────────────────────────────────┘
   BUZZER NOTE:
   tone() generates a real audio frequency square wave so Wokwi's
   piezo buzzer actually produces sound.
   noTone() stops it cleanly.
   The siren alternates 800 Hz / 1400 Hz every 400 ms using
   millis() — no blocking delay() needed.

   THRESHOLD NOTE:
   INTRUSION_DISTANCE_CM = 400 cm for easy Wokwi demo.
   Drag the HC-SR04 slider below 400 cm to trigger.
   Change to 15.0 for real hardware deployment.
   ============================================================ */

// ---------- BLYNK CREDENTIALS ----------
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

// ---------- WIFI ----------
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// ---------- PIN DEFINITIONS ----------
#define TRIG_PIN      5
#define ECHO_PIN      18
#define DHT_PIN       4
#define LDR_PIN       34
#define SERVO_PIN     13
#define BUZZER_PIN    25
#define RED_PIN       26
#define GREEN_PIN     27
#define BLUE_PIN      14
#define RELAY_PIN     32

#define DHT_TYPE      DHT22
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

// ---------- THRESHOLDS ----------
const float INTRUSION_DISTANCE_CM = 400.0;  // cm  ← DEMO (change to 15.0 for real hardware)
const float FIRE_TEMP_THRESHOLD   = 40.0;   // °C
const int   DARK_THRESHOLD        = 1500;   // LDR ADC raw

// ---------- SIREN TONE SETTINGS ----------
const int   SIREN_FREQ_LOW   = 800;    // Hz - low tone
const int   SIREN_FREQ_HIGH  = 1400;   // Hz - high tone
const unsigned long SIREN_TOGGLE_MS = 400;  // switch tone every 400ms

// ---------- OBJECTS ----------
DHT dht(DHT_PIN, DHT_TYPE);
Servo doorLockServo;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- STATE ----------
bool doorLocked           = true;
bool manualLightOn        = false;
bool alarmActive          = false;
bool sirenHighTone        = false;
unsigned long lastSensorRead   = 0;
unsigned long lastSirenToggle  = 0;
const unsigned long SENSOR_INTERVAL = 2000;

// ============================================================
//  HELPERS
// ============================================================

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return 999.0;
  return duration * 0.0343 / 2.0;
}

void setRGB(bool r, bool g, bool b) {
  digitalWrite(RED_PIN,   r ? HIGH : LOW);
  digitalWrite(GREEN_PIN, g ? HIGH : LOW);
  digitalWrite(BLUE_PIN,  b ? HIGH : LOW);
}

// ---- Continuous alternating siren — call every loop tick ----
void runSiren() {
  if (!alarmActive) return;
  if (millis() - lastSirenToggle >= SIREN_TOGGLE_MS) {
    lastSirenToggle = millis();
    sirenHighTone   = !sirenHighTone;
    tone(BUZZER_PIN, sirenHighTone ? SIREN_FREQ_HIGH : SIREN_FREQ_LOW);
  }
}

void lockDoor() {
  doorLockServo.write(0);
  doorLocked = true;
  Blynk.virtualWrite(V4, 1);
  Serial.println("  >> SERVO  : LOCKED  (0 deg)");
}

void unlockDoor() {
  doorLockServo.write(90);
  doorLocked = false;
  Blynk.virtualWrite(V4, 0);
  Serial.println("  >> SERVO  : UNLOCKED (90 deg)");
}

void triggerAlarm(String reason) {
  if (!alarmActive) {
    alarmActive     = true;
    sirenHighTone   = false;
    lastSirenToggle = 0;          // fire immediately on next runSiren()
    Serial.println("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Serial.println("  !! ALARM TRIGGERED: " + reason);
    Serial.println("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    Blynk.logEvent("security_alert", reason);
  }
  // tone() started by runSiren() on next loop tick
  setRGB(true, false, false);     // RGB → RED
  lockDoor();
}

void clearAlarm() {
  alarmActive = false;
  noTone(BUZZER_PIN);             // ← stops the siren cleanly
  setRGB(false, true, false);     // RGB → GREEN
  Serial.println("  >> ALARM CLEARED. Status: SAFE");
}

// ============================================================
//  BLYNK VIRTUAL PIN HANDLERS
// ============================================================

BLYNK_WRITE(V4) {
  int v = param.asInt();
  if (!alarmActive) { if (v == 1) lockDoor(); else unlockDoor(); }
  else Blynk.virtualWrite(V4, 1);
}

BLYNK_WRITE(V5) {
  manualLightOn = param.asInt();
  digitalWrite(RELAY_PIN, manualLightOn ? HIGH : LOW);
  Serial.println(manualLightOn ? "  >> RELAY  : MANUAL ON" : "  >> RELAY  : MANUAL OFF");
}

BLYNK_WRITE(V7) {
  if (param.asInt() == 1) { clearAlarm(); unlockDoor(); }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("============================================");
  Serial.println("  Student   : Anik Adnan");
  Serial.println("  ID        : <<REPLACE_WITH_YOUR_STUDENT_ID>>");
  Serial.println("  Project   : Smart IoT Security System");
  Serial.println("  Threshold : " + String(INTRUSION_DISTANCE_CM) + " cm (DEMO)");
  Serial.println("  Buzzer    : tone() siren 800/1400 Hz alternating");
  Serial.println("  HOW TO TEST in Wokwi:");
  Serial.println("    1. Click HC-SR04 → drag distance below 400 cm");
  Serial.println("    2. Click DHT22   → raise temp above 40 C");
  Serial.println("    3. Click LDR     → lower light for night mode");
  Serial.println("    4. Tap V7 in Blynk app to reset alarm");
  Serial.println("============================================");

  pinMode(TRIG_PIN,   OUTPUT);
  pinMode(ECHO_PIN,   INPUT);
  pinMode(RED_PIN,    OUTPUT);
  pinMode(GREEN_PIN,  OUTPUT);
  pinMode(BLUE_PIN,   OUTPUT);
  pinMode(RELAY_PIN,  OUTPUT);
  pinMode(LDR_PIN,    INPUT);

  noTone(BUZZER_PIN);               // ensure silent on boot
  dht.begin();
  doorLockServo.attach(SERVO_PIN);
  lockDoor();
  setRGB(false, true, false);       // GREEN = safe
  digitalWrite(RELAY_PIN, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED init failed");
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart IoT Security");
  display.println("BOOTING...");
  display.println("Siren: 800/1400 Hz");
  display.display();

  Blynk.begin(auth, ssid, pass);
  Serial.println("  System READY. Monitoring...");
  Serial.println("============================================");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  Blynk.run();
  runSiren();      // ← keeps the siren running continuously between sensor reads

  if (millis() - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = millis();

    float distance    = readDistanceCM();
    float temperature = dht.readTemperature();
    float humidity    = dht.readHumidity();
    int   lightLevel  = analogRead(LDR_PIN);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("  [WARN] DHT22 read failed.");
      temperature = 0; humidity = 0;
    }

    // ---- Push to Blynk ----
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, distance);
    Blynk.virtualWrite(V3, lightLevel);

    // ---- Auto room light via LDR ----
    if (!manualLightOn) {
      digitalWrite(RELAY_PIN, (lightLevel < DARK_THRESHOLD) ? HIGH : LOW);
    }

    // ---- ALARM LOGIC ----
    if (distance < INTRUSION_DISTANCE_CM && distance < 999.0) {
      triggerAlarm("INTRUSION: " + String(distance, 1) + " cm < " + String(INTRUSION_DISTANCE_CM, 0) + " cm");
    }
    else if (temperature > FIRE_TEMP_THRESHOLD) {
      triggerAlarm("FIRE RISK: " + String(temperature, 1) + " C > " + String(FIRE_TEMP_THRESHOLD, 0) + " C");
    }
    else if (!alarmActive) {
      if (lightLevel < DARK_THRESHOLD) {
        setRGB(false, false, true);   // BLUE = night safe
      } else {
        setRGB(false, true, false);   // GREEN = day safe
      }
    }

    // ---- Serial Monitor ----
    Serial.println("---------------------------------------------");
    Serial.println("  Anik Adnan | <<REPLACE_WITH_YOUR_STUDENT_ID>>");
    Serial.print  ("  Distance  : "); Serial.print(distance, 2); Serial.print(" cm");
    Serial.println((distance < INTRUSION_DISTANCE_CM && distance < 999.0) ? "  <<< INTRUSION!" : "  [clear]");
    Serial.print  ("  Temp      : "); Serial.print(temperature, 1); Serial.print(" C");
    Serial.println(temperature > FIRE_TEMP_THRESHOLD ? "  <<< FIRE RISK!" : "");
    Serial.print  ("  Humidity  : "); Serial.print(humidity, 1);   Serial.println(" %");
    Serial.print  ("  Light LDR : "); Serial.print(lightLevel);
    Serial.println(lightLevel < DARK_THRESHOLD ? "  (DARK - relay ON)" : "  (bright)");
    Serial.print  ("  Door Lock : "); Serial.println(doorLocked  ? "LOCKED"       : "UNLOCKED");
    Serial.print  ("  Alarm     : "); Serial.println(alarmActive ? "!! ACTIVE !!" : "SAFE");
    Serial.print  ("  Buzzer    : "); Serial.println(alarmActive ? (sirenHighTone ? "1400 Hz HIGH TONE" : "800 Hz LOW TONE") : "OFF");
    Serial.print  ("  LED Color : ");
    if (alarmActive)                  Serial.println("RED   (alarm)");
    else if (lightLevel < DARK_THRESHOLD) Serial.println("BLUE  (night-safe)");
    else                              Serial.println("GREEN (day-safe)");

    // ---- OLED ----
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("STAT:");
    display.println(alarmActive ? " !! ALERT !!" : " SAFE");
    display.print("Dist:"); display.print(distance, 1); display.println("cm");
    display.print("Temp:"); display.print(temperature, 1); display.println("C");
    display.print("Hum :"); display.print(humidity, 1); display.println("%");
    display.print("Lght:"); display.println(lightLevel);
    display.print("Lock:"); display.println(doorLocked ? "LOCKED" : "OPEN");
    display.print("Buzz:");
    display.println(alarmActive ? (sirenHighTone ? "1400Hz" : "800Hz") : "OFF");
    display.display();
  }
}
