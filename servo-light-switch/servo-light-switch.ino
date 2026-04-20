#include <ESP32Servo.h>
#include <HomeSpan.h>
#include <WiFi.h>

static constexpr int SERVO_PIN               = 3;
static constexpr int SERVO_REST_ANGLE        = 15;
static constexpr int SERVO_PRESS_ANGLE       = 35;
static constexpr int PRESS_HOLD_MS           = 200;
static constexpr unsigned long WIFI_STUCK_REBOOT_MS = 60000;

static Servo g_servo;
static unsigned long g_lastWifiConnected = 0;

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

struct DormLight : Service::LightBulb {
  SpanCharacteristic *power;
  unsigned long resetAt = 0;

  DormLight() : Service::LightBulb() {
    power = new Characteristic::On(false);
  }

  boolean update() override {
    if (power->getNewVal<bool>()) {
      Serial.println("homekit: turn-on requested, pressing button");
      pressButton();
      resetAt = millis() + 300;
    }
    return true;
  }

  void loop() override {
    if (resetAt != 0 && millis() > resetAt) {
      power->setVal(false);
      resetAt = 0;
    }
  }
};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Serial.println("servo-light-switch: boot OK");

  g_servo.attach(SERVO_PIN);
  g_servo.write(SERVO_REST_ANGLE);
  delay(500);
  g_servo.detach();
  Serial.println("servo: initialised at rest");

  homeSpan.setLogLevel(1);
  homeSpan.begin(Category::Lighting, "Dorm Light");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Dorm Light");
      new Characteristic::Manufacturer("DIY");
      new Characteristic::Model("ServoLightSwitch");
      new Characteristic::SerialNumber("SLS-0001");
      new Characteristic::FirmwareRevision("1.0");
    new DormLight();

  g_lastWifiConnected = millis();
  Serial.println("homekit: ready");
}

void loop() {
  homeSpan.poll();

  if (WiFi.status() == WL_CONNECTED) {
    g_lastWifiConnected = millis();
  } else if (millis() - g_lastWifiConnected > WIFI_STUCK_REBOOT_MS) {
    Serial.println("watchdog: WiFi down >60s, rebooting");
    delay(100);
    ESP.restart();
  }
}
