// Servo Light Switch — XIAO ESP32S3 (Sense)
//
// A servo arm flips a Decora rocker paddle on command from Apple HomeKit.
// Unlike the old dorm push-button, a rocker has two stable states, so the arm
// swings UP to press the top half (ON) or DOWN to press the bottom half (OFF),
// resting in the middle when idle. The servo is the only thing that moves the
// switch, so HomeKit's stored state stays true to the wall — and we persist it
// in NVS so it survives reboots.
//
// NOTE: SERVO_PIN is GPIO2, which is the pad labelled "D1" on the XIAO ESP32S3
// silkscreen. (On the older C3 board, D1 was GPIO3 — same pad, different GPIO.)

#include <ESP32Servo.h>
#include <HomeSpan.h>
#include <WiFi.h>

// ---- Switch geometry — tune these to your servo + switch (see README) -------
static constexpr int SERVO_PIN        = 2;    // D1 on the silkscreen = GPIO2
static constexpr int SERVO_REST_ANGLE = 90;   // arm centered, clear of the paddle
static constexpr int SERVO_ON_ANGLE   = 130;  // arm presses the TOP half  -> light ON
static constexpr int SERVO_OFF_ANGLE  = 50;   // arm presses the BOTTOM half -> light OFF
static constexpr int PRESS_HOLD_MS    = 250;  // how long to push the paddle
static constexpr int SETTLE_MS        = 200;  // pause back at rest before releasing
static constexpr unsigned long WIFI_STUCK_REBOOT_MS = 60000;

static Servo g_servo;
static unsigned long g_lastWifiConnected = 0;

// Swing the arm to `angle`, hold to flip the paddle, return to rest, then detach
// so the servo doesn't buzz or draw current while idle.
static void flip(int angle, const char *label) {
  Serial.printf("servo: flip %s\n", label);
  g_servo.attach(SERVO_PIN);
  g_servo.write(angle);
  delay(PRESS_HOLD_MS);
  g_servo.write(SERVO_REST_ANGLE);
  delay(SETTLE_MS);
  g_servo.detach();
}

static void flipOn()  { flip(SERVO_ON_ANGLE,  "ON (press top)");     }
static void flipOff() { flip(SERVO_OFF_ANGLE, "OFF (press bottom)"); }

struct WallSwitch : Service::LightBulb {
  SpanCharacteristic *power;

  WallSwitch() : Service::LightBulb() {
    // Second arg `true` => save the value in NVS and restore it on reboot, so
    // HomeKit shows the real last state after a power cut.
    power = new Characteristic::On(false, true);
  }

  boolean update() override {
    bool on = power->getNewVal<bool>();
    Serial.printf("homekit: turn %s requested\n", on ? "ON" : "OFF");
    if (on) flipOn();
    else    flipOff();
    return true;
  }
};

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  Serial.println("servo-light-switch: boot OK");

  // Park the arm in the neutral position — clear of both halves, touching neither.
  g_servo.attach(SERVO_PIN);
  g_servo.write(SERVO_REST_ANGLE);
  delay(500);
  g_servo.detach();
  Serial.println("servo: parked at rest");

  homeSpan.setLogLevel(1);
  homeSpan.begin(Category::Lighting, "Wall Light");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("Wall Light");
      new Characteristic::Manufacturer("DIY");
      new Characteristic::Model("ServoLightSwitch S3");
      new Characteristic::SerialNumber("SLS-0002");
      new Characteristic::FirmwareRevision("2.0");
    new WallSwitch();

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
