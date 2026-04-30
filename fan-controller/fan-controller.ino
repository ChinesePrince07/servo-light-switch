// Fan controller: XIAO ESP32C3 + servo + DHT22 + SSD1306 OLED + HomeKit (HomeSpan)
//
// Fan cycle (single physical button): OFF -> LOW -> MID -> HIGH -> OFF -> ...
// HomeKit exposes: Fanv2 (speed control), TemperatureSensor, and a Switch for Auto Mode.
//
// Required libraries (install via Arduino Library Manager):
//   HomeSpan, ESP32Servo,
//   Adafruit DHT Sensor Library, Adafruit Unified Sensor,
//   Adafruit SSD1306, Adafruit GFX Library
//
// Board: XIAO_ESP32C3  (USB CDC On Boot: Enabled,
//         Partition: Minimal SPIFFS 1.9MB APP w/ OTA / 190KB SPIFFS)
//
// Wiring:
//   3.3V  -> Servo VCC, DHT22 pin-1, OLED VCC
//   GND   -> Servo GND, DHT22 pin-4, OLED GND
//   GPIO3 -> Servo PWM
//   GPIO4 -> DHT22 data (add 10kΩ pull-up to 3.3V)
//   GPIO6 -> OLED SDA
//   GPIO7 -> OLED SCL

#include <ESP32Servo.h>
#include <HomeSpan.h>
#include <WiFi.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Pin / hardware constants ──────────────────────────────────────────────────
static constexpr int   SERVO_PIN         = 3;
static constexpr int   SERVO_REST_ANGLE  = 15;
static constexpr int   SERVO_PRESS_ANGLE = 40;
static constexpr int   PRESS_HOLD_MS     = 200;
static constexpr int   PRESS_PAUSE_MS    = 600;

static constexpr int   DHT_PIN           = 4;
static constexpr int   DHT_TYPE          = DHT22;

static constexpr int   OLED_SDA         = 6;
static constexpr int   OLED_SCL         = 7;
static constexpr uint8_t OLED_ADDR      = 0x3C;
static constexpr int   SCREEN_WIDTH     = 128;
static constexpr int   SCREEN_HEIGHT    = 64;

// ── Timing constants ──────────────────────────────────────────────────────────
static constexpr unsigned long AUTO_INTERVAL_MS     = 30000;
static constexpr unsigned long WIFI_STUCK_REBOOT_MS = 60000;

// ── Temperature thresholds (°C) ───────────────────────────────────────────────
static constexpr float TEMP_THRESH_OFF = 18.0f;
static constexpr float TEMP_THRESH_LOW = 22.0f;
static constexpr float TEMP_THRESH_MID = 26.0f;

// ── Hardware objects ──────────────────────────────────────────────────────────
static Servo               g_servo;
static DHT                 g_dht(DHT_PIN, DHT_TYPE);
static Adafruit_SSD1306    g_display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ── Fan state ─────────────────────────────────────────────────────────────────
enum FanState : uint8_t { FAN_OFF = 0, FAN_LOW = 1, FAN_MID = 2, FAN_HIGH = 3 };

static FanState        g_currentFanState = FAN_OFF;
static float           g_lastTempC       = NAN;
static unsigned long   g_lastAutoCheck   = 0;
static unsigned long   g_lastWifiConnected = 0;

// ── Forward declarations ──────────────────────────────────────────────────────
static void     pressButton();
static void     setFanState(FanState target);
static float    readTemperature();
static void     updateDisplay(float tempC, FanState state);
static FanState tempToFanState(float tempC);
static const char* fanStateLabel(FanState s);

// ── HomeSpan service pointers (set in setup) ──────────────────────────────────
struct DormFan;
struct TempSensor;
struct AutoMode;

static DormFan    *g_fanService  = nullptr;
static TempSensor *g_tempService = nullptr;
static AutoMode   *g_autoService = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
// HomeSpan service: Fanv2
// ─────────────────────────────────────────────────────────────────────────────
struct DormFan : Service::Fanv2 {
  SpanCharacteristic *active;
  SpanCharacteristic *speed;   // RotationSpeed 0–100 %

  DormFan() : Service::Fanv2() {
    active = new Characteristic::Active(0);
    speed  = new Characteristic::RotationSpeed(0);
    speed->setRange(0, 100, 1);
  }

  boolean update() override {
    bool   isActive  = active->getNewVal<bool>();
    float  newSpeed  = speed->isUpdated() ? speed->getNewVal<float>() : speed->getVal<float>();

    FanState target;
    if (!isActive || newSpeed == 0) {
      target = FAN_OFF;
    } else if (newSpeed <= 33) {
      target = FAN_LOW;
    } else if (newSpeed <= 66) {
      target = FAN_MID;
    } else {
      target = FAN_HIGH;
    }

    // If turning on without a speed, default to LOW
    if (isActive && target == FAN_OFF) {
      target = FAN_LOW;
      speed->setVal(33.0f);
    }

    setFanState(target);
    updateDisplay(g_lastTempC, g_currentFanState);
    return true;
  }

  // Called by auto-adjust to push fan state into HomeKit without re-triggering update()
  void syncFromState(FanState s) {
    bool on = (s != FAN_OFF);
    float pct = 0;
    switch (s) {
      case FAN_LOW:  pct = 33;  break;
      case FAN_MID:  pct = 66;  break;
      case FAN_HIGH: pct = 100; break;
      default:       pct = 0;   break;
    }
    active->setVal(on ? 1 : 0);
    speed->setVal(pct);
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// HomeSpan service: TemperatureSensor
// ─────────────────────────────────────────────────────────────────────────────
struct TempSensor : Service::TemperatureSensor {
  SpanCharacteristic *currentTemp;

  TempSensor() : Service::TemperatureSensor() {
    currentTemp = new Characteristic::CurrentTemperature(20.0f);
    currentTemp->setRange(-40, 80);
  }

  void updateTemp(float celsius) {
    if (!isnan(celsius)) {
      currentTemp->setVal(celsius);
    }
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// HomeSpan service: Switch (Auto Mode)
// ─────────────────────────────────────────────────────────────────────────────
struct AutoMode : Service::Switch {
  SpanCharacteristic *autoOn;

  AutoMode() : Service::Switch() {
    autoOn = new Characteristic::On(true);  // auto mode ON by default
  }

  boolean update() override { return true; }

  bool isEnabled() { return autoOn->getVal<bool>(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// Servo helpers
// ─────────────────────────────────────────────────────────────────────────────
static void pressButton() {
  Serial.println("servo: press");
  g_servo.attach(SERVO_PIN);
  g_servo.write(SERVO_PRESS_ANGLE);
  delay(PRESS_HOLD_MS);
  g_servo.write(SERVO_REST_ANGLE);
  delay(PRESS_HOLD_MS);
  g_servo.detach();
  Serial.println("servo: released");
}

static void setFanState(FanState target) {
  if (target == g_currentFanState) return;

  uint8_t presses = (target - g_currentFanState + 4) % 4;
  Serial.printf("fan: %s -> %s (%d press%s)\n",
    fanStateLabel(g_currentFanState), fanStateLabel(target),
    presses, presses == 1 ? "" : "es");

  for (uint8_t i = 0; i < presses; i++) {
    if (i > 0) delay(PRESS_PAUSE_MS);
    pressButton();
  }
  g_currentFanState = target;
}

// ─────────────────────────────────────────────────────────────────────────────
// DHT helper
// ─────────────────────────────────────────────────────────────────────────────
static float readTemperature() {
  return g_dht.readTemperature();
}

// ─────────────────────────────────────────────────────────────────────────────
// Temperature → fan state mapping
// ─────────────────────────────────────────────────────────────────────────────
static FanState tempToFanState(float tempC) {
  if (tempC < TEMP_THRESH_OFF) return FAN_OFF;
  if (tempC < TEMP_THRESH_LOW) return FAN_LOW;
  if (tempC < TEMP_THRESH_MID) return FAN_MID;
  return FAN_HIGH;
}

// ─────────────────────────────────────────────────────────────────────────────
// Display helpers
// ─────────────────────────────────────────────────────────────────────────────
static const char* fanStateLabel(FanState s) {
  switch (s) {
    case FAN_OFF:  return "OFF";
    case FAN_LOW:  return "LOW";
    case FAN_MID:  return "MID";
    case FAN_HIGH: return "HIGH";
    default:       return "???";
  }
}

static void updateDisplay(float tempC, FanState state) {
  g_display.clearDisplay();
  g_display.setTextColor(SSD1306_WHITE);
  g_display.setTextSize(2);

  g_display.setCursor(0, 0);
  g_display.print("TEMP:");
  if (isnan(tempC)) {
    g_display.print("--.-C");
  } else {
    g_display.print(tempC, 1);
    g_display.print("C");
  }

  g_display.setCursor(0, 34);
  g_display.print("FAN: ");
  g_display.print(fanStateLabel(state));

  g_display.display();
}

// ─────────────────────────────────────────────────────────────────────────────
// setup / loop
// ─────────────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Serial.println("fan-controller: boot OK");

  // Servo init
  g_servo.attach(SERVO_PIN);
  g_servo.write(SERVO_REST_ANGLE);
  delay(500);
  g_servo.detach();
  Serial.println("servo: initialised at rest");

  // DHT init
  g_dht.begin();
  Serial.println("dht22: initialised");

  // OLED init
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!g_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("oled: init failed — continuing without display");
  } else {
    g_display.clearDisplay();
    g_display.display();
    Serial.println("oled: initialised");
    updateDisplay(NAN, FAN_OFF);
  }

  // HomeSpan init
  homeSpan.setLogLevel(1);
  homeSpan.begin(Category::Fans, "Dorm Fan");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Dorm Fan");
      new Characteristic::Manufacturer("DIY");
      new Characteristic::Model("FanController");
      new Characteristic::SerialNumber("FC-0001");
      new Characteristic::FirmwareRevision("1.0");
    g_fanService  = new DormFan();
    g_tempService = new TempSensor();
    g_autoService = new AutoMode();

  g_lastAutoCheck    = millis();
  g_lastWifiConnected = millis();
  Serial.println("homekit: ready");
}

void loop() {
  homeSpan.poll();

  // Auto temperature control
  if (millis() - g_lastAutoCheck >= AUTO_INTERVAL_MS) {
    g_lastAutoCheck = millis();

    float tempC = readTemperature();
    g_lastTempC = tempC;

    if (g_tempService) g_tempService->updateTemp(tempC);

    if (!isnan(tempC)) {
      Serial.printf("temp: %.1f°C\n", tempC);
      if (g_autoService && g_autoService->isEnabled()) {
        FanState target = tempToFanState(tempC);
        setFanState(target);
        if (g_fanService) g_fanService->syncFromState(g_currentFanState);
      }
    } else {
      Serial.println("temp: DHT read failed, skipping auto-adjust");
    }

    updateDisplay(g_lastTempC, g_currentFanState);
  }

  // WiFi watchdog
  if (WiFi.status() == WL_CONNECTED) {
    g_lastWifiConnected = millis();
  } else if (millis() - g_lastWifiConnected > WIFI_STUCK_REBOOT_MS) {
    Serial.println("watchdog: WiFi down >60s, rebooting");
    delay(100);
    ESP.restart();
  }
}
