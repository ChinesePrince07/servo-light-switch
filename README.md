# Servo Light Switch

A voice-controlled dorm light switch. A XIAO ESP32C3 exposes itself as a HomeKit Lightbulb over WiFi; Siri commands trigger an MG90S servo that presses the physical switch.

"Hey Siri, turn on the dorm light" → servo presses button.

## Hardware

- **XIAO ESP32C3** (with external IPEX 2.4GHz antenna — internal antenna is too weak for reliable HomeKit)
- **MG90S** micro servo
- USB-C power (LiPo on the BAT pads works too)

### Wiring

| XIAO C3 pin   | Servo wire      |
|---------------|-----------------|
| 3.3V          | Red (VCC)       |
| GND           | Brown (GND)     |
| D1 (GPIO3)    | Orange (Signal) |

Adhesive-mount the assembly next to the switch so the servo horn reaches the button.

## Software

**Arduino IDE** with the `esp32` board package (v3.0.0+). Required libraries:

- `ESP32Servo`
- `HomeSpan`

### Board settings

- Board: **XIAO_ESP32C3**
- USB CDC On Boot: **Enabled**
- Partition Scheme: **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** — required, HomeSpan won't fit the default scheme

### First-time setup

1. Flash the sketch
2. Open Serial Monitor at 115200 baud, line-ending = Newline
3. Type `W` to configure WiFi (2.4GHz only — C3 does not support 5GHz)
4. After reboot, note the HomeKit setup code (default: `466-37-726`)
5. iPhone Home app → Add Accessory → Dorm Light → enter setup code

## Calibration

Tune these constants in the sketch to match your switch geometry:

| Constant            | Value | Purpose                                   |
|---------------------|-------|-------------------------------------------|
| `SERVO_REST_ANGLE`  | 15°   | Arm clears the button at rest             |
| `SERVO_PRESS_ANGLE` | 35°   | Arm swings to press the button            |
| `PRESS_HOLD_MS`     | 200   | How long the press is held                |
| `SERVO_PIN`         | 3     | GPIO3 (D1 on silkscreen)                  |

## Behaviour

The switch is a single toggle button — one press toggles the light regardless of state. The sketch mirrors whatever on/off state HomeKit believes and fires one servo press on every ON↔OFF transition.

If the button is toggled manually (not via Siri), HomeKit's state drifts from reality. The next voice command matching the stale state is a no-op; the one after resyncs.

## WiFi watchdog

If WiFi stays disconnected for 60 seconds, the sketch reboots the board. Prevents the "stuck unavailable" state after a drop.

## Troubleshooting

- **"Unavailable" in Home app / Siri unresponsive:** signal is likely weak. Check `RSSI` in Serial on reconnect. Below -75 is marginal. External IPEX antenna on the XIAO helps a lot.
- **WiFi won't connect:** make sure it's 2.4GHz; C3 cannot see 5GHz networks.
- **Board resets during servo movement:** press angle is causing the servo to stall against the switch. Reduce `SERVO_PRESS_ANGLE`.
- **HomeKit works on hotspot but not your network:** the network likely has AP/client isolation blocking local discovery. A travel router or your own router fixes it.
