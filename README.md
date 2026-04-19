# Servo Light Switch

Got a light switch that's on the wrong side of the room? Don't want to get out of bed? Same. This is a tiny robot finger that presses the switch when you ask Siri nicely.

A XIAO ESP32C3 advertises itself on WiFi as an Apple HomeKit light. Siri sends the on/off command, the board wiggles an MG90S servo, and the servo pokes the button. That's the whole trick.

> "Hey Siri, turn on the light."

https://github.com/user-attachments/assets/30583818-0d01-4782-8400-2a43bfde717b



## Hardware

- XIAO ESP32C3 (with an external IPEX 2.4 GHz antenna — the onboard one is optimistic)
- MG90S micro servo
- USB-C power, or a single-cell LiPo on the BAT pads
- Double-sided tape or command strips for mounting

### Wiring

| XIAO C3 pin | Servo wire    |
|-------------|---------------|
| 3.3V        | Red (VCC)     |
| GND         | Brown (GND)   |
| D1 (GPIO3)  | Orange (PWM)  |

Stick the whole thing next to the switch so the servo horn can reach the button. That's it.

## Software

Arduino IDE, with the `esp32` board package (3.0.0+). Libraries:

- `ESP32Servo`
- `HomeSpan`

### Board settings that matter

- Board: **XIAO_ESP32C3**
- USB CDC On Boot: **Enabled**
- Partition Scheme: **Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** — HomeSpan is chonky and won't fit the default scheme

### First-time setup

1. Flash the sketch.
2. Open Serial Monitor at 115200, line-ending = Newline.
3. Type `W` to give it WiFi credentials. Must be 2.4 GHz — the C3 ignores 5 GHz like it's beneath it.
4. Note the HomeKit setup code (default: `466-37-726`).
5. iPhone → Home app → Add Accessory → pick "Dorm Light" → enter the code. Ignore the "Uncertified Accessory" warning; that's normal for DIY.

## Calibration

Tune these constants at the top of the sketch to your particular switch:

| Constant            | Value | Does what                          |
|---------------------|-------|------------------------------------|
| `SERVO_REST_ANGLE`  | 15°   | Arm position at rest               |
| `SERVO_PRESS_ANGLE` | 35°   | Arm position while pressing        |
| `PRESS_HOLD_MS`     | 200   | How long it holds the button       |
| `SERVO_PIN`         | 3     | GPIO3 (D1 on silkscreen)           |

If the servo misses the button, bump the press angle up. If it slams and buzzes, bump it down. If the light isn't actuating, hold it longer.

## How it behaves

The switch is a single toggle button — one press flips the light, no matter which way. The code mirrors whatever on/off state HomeKit thinks the light is in and fires a press on every on↔off flip.

If someone toggles the switch manually, HomeKit's state drifts. The next voice command in the "wrong" direction does nothing; the one after it resyncs. Mildly annoying, easy to work around.

## WiFi watchdog

If WiFi is down for 60 seconds, the board reboots itself. This dodges the "stuck unavailable" state that Home app sometimes gets into after a network hiccup.

## Troubleshooting

- **"Unavailable" in Home app:** probably weak signal. Check `RSSI` in Serial when it reconnects. Below -75 is rough; -85 is broken. Get an external antenna — it's a huge upgrade.
- **WiFi just won't connect:** confirm it's 2.4 GHz. The C3 cannot see 5 GHz.
- **Board resets when the servo moves:** the servo is stalling and browning out the regulator. Reduce the press angle.
- **Works perfectly on a hotspot, flaky everywhere else:** your network has client isolation (common on dorm/guest WiFi). A $25 travel router fixes it and is worth the trouble.
