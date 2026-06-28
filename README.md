# Servo Light Switch

A light switch across the room and a strong disinclination to walk over to it. This is a tiny robot finger that flips the switch when you ask Siri nicely.

A XIAO ESP32S3 advertises itself on WiFi as an Apple HomeKit light. Siri sends the on/off command, the board swings an MG90S servo, and the servo arm flips the rocker paddle — up for on, down for off. That's the whole trick.

> "Hey Siri, turn on the light."

https://github.com/user-attachments/assets/30583818-0d01-4782-8400-2a43bfde717b

New to a soldering iron? Start with **[SOLDERING.md](SOLDERING.md)** — it walks the three joints in this build from zero.

## Hardware

- **XIAO ESP32S3** (the Sense version is fine — its camera/mic just go unused here) with the external IPEX 2.4 GHz antenna attached. The onboard antenna is optimistic.
- **MG90S** micro servo
- **USB-C** power — a decent wall adapter (≥1 A, 2 A is comfortable). The servo pulls current spikes when it moves, and a real wall switch is stiffer than a dorm push-button, so we run the servo off 5 V for full torque.
- Three short lengths of wire (or just the servo's own leads), and something to mount it: a 3D-printed bracket, double-sided foam tape, or command strips.
- *Optional:* a 470–1000 µF electrolytic capacitor across the servo's V+/GND to smooth current spikes and dodge brown-out reboots.

### Wiring

| Servo wire    | XIAO ESP32S3 pad | GPIO   |
|---------------|------------------|--------|
| Red (V+)      | **5V**           | —      |
| Brown (GND)   | **GND**          | —      |
| Orange (PWM)  | **D1**           | GPIO2  |

> **Coming from the C3?** `D1` is the *same physical pad*, but on the S3 it's **GPIO2**, not GPIO3 — so `SERVO_PIN` in the sketch is `2`, not `3`. Also note the servo's red wire now goes to **5V** (not 3.3V) for more torque. If you've got the capacitor, its legs go to the same 5V and GND pads (mind the polarity — the stripe is the negative leg, to GND).

The PWM signal is 3.3 V logic, which the MG90S reads fine even while powered from 5 V.

### Mounting

The arm swings from a center rest position to press one half of the paddle:

```
        ┌───────┐
ON  ◄── │  ▲    │   arm swings up,   presses TOP half
        │┈┈┈┈┈┈┈│   arm rests center, clear of paddle
OFF ◄── │    ▼  │   arm swings down, presses BOTTOM half
        └───────┘
```

Mount the servo beside the switch plate so the horn's pivot sits roughly level with the paddle's middle and a stiff arm/horn can reach across to either half. Stick the whole thing down so the arm has firm leverage against the paddle. Then tune the angles below.

## Software

Arduino IDE, with the `esp32` board package (3.0.0+). Libraries:

- `ESP32Servo`
- `HomeSpan`

### Board settings that matter

- Board: **XIAO_ESP32S3**
- USB CDC On Boot: **Enabled** (so the Serial Monitor works over USB)
- PSRAM: **OPI PSRAM**
- Partition Scheme: **default is fine.** The S3's 8 MB flash has room for HomeSpan — no need for the "Minimal SPIFFS" workaround the 4 MB C3 required.

> If the board doesn't show up as a serial port for flashing, put it in bootloader mode: hold **BOOT**, tap **RESET**, release **BOOT**.

### First-time setup

1. Flash the sketch.
2. Open Serial Monitor at 115200, line-ending = Newline.
3. Type `W` to give it WiFi credentials. Must be 2.4 GHz — the S3 has no 5 GHz radio.
4. Note the HomeKit setup code (default: `466-37-726`).
5. iPhone → Home app → Add Accessory → pick "Wall Light" → enter the code. Ignore the "Uncertified Accessory" warning; that's normal for DIY.

## Calibration

Tune these constants at the top of the sketch to your particular switch and how the servo is mounted:

| Constant            | Value | Does what                                   |
|---------------------|-------|---------------------------------------------|
| `SERVO_REST_ANGLE`  | 90°   | Arm centered, clear of the paddle           |
| `SERVO_ON_ANGLE`    | 130°  | Arm position that presses the **top** half  |
| `SERVO_OFF_ANGLE`   | 50°   | Arm position that presses the **bottom** half |
| `PRESS_HOLD_MS`     | 250   | How long it holds the paddle                |
| `SERVO_PIN`         | 2     | GPIO2 (D1 on the silkscreen)                |

Start by finding `SERVO_REST_ANGLE` where the arm sits clear of the paddle, then bump `SERVO_ON_ANGLE` / `SERVO_OFF_ANGLE` out from there until each press flips the rocker cleanly. If the servo slams and buzzes against the paddle, back the angle off toward rest. If it doesn't fully flip the switch, push the angle further or hold longer.

> **On/off reversed?** If "turn on" turns it *off*, just swap the values of `SERVO_ON_ANGLE` and `SERVO_OFF_ANGLE` — it depends on which way round the servo is mounted.

## How it behaves

Your rocker has two real, stable states, so HomeKit treats it as a normal light:

- **"Turn on"** → arm presses the top half. **"Turn off"** → arm presses the bottom half. Both directions work.
- The servo is the only thing that moves the switch, so HomeKit's state matches the wall. That state is saved to NVS, so it's still correct after a reboot or power cut.
- Pressing is harmless when it's already in that state — "turn on" while it's on just nudges the top again — so a redundant command self-corrects.

**One limitation:** if *you* flip the switch by hand, the board doesn't know. HomeKit still thinks it's in the old state, and because it thinks so, Siri may swallow the next same-direction command ("it's already on"). Just give the opposite command, or toggle twice. (The Sense version's camera could one day watch the room and detect manual flips — that's a future upgrade, not part of this build.)

## WiFi watchdog

If WiFi is down for 60 seconds, the board reboots itself. This dodges the "stuck unavailable" state the Home app sometimes lands in after a network hiccup.
