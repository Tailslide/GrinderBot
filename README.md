# GrinderBot

A scale that sits under a Mazzer Mini Electronic (Type A) and grinds by weight: a servo presses and holds the grinder's manual button, and lets go when the cup reaches the dose. Arduino MKR WiFi 1010, HX711 + 1 kg load cell, 128×32 OLED, four touch pads, a piezo buzzer and an MG90S servo. PlatformIO project.

## Using it

| Pad | On the weight screen | In menus and settings |
|---|---|---|
| **OK** | Grind the selected dose (put the cup on first; it tares, ~0.7 s) | Select / save |
| **1 ▲** | Tap: dose 1. Hold 2 s: edit dose 1 | Up / more |
| **2 ▼** | Tap: dose 2. Hold 2 s: edit dose 2 | Down / less |
| **≡** | Tap: tare. Hold 2 s: menu | Back / cancel |

An OK touch that overlaps a touch on another pad is ignored, and a grind starts 150 ms after OK is let go only if no other pad is touched in that time. This stops a finger on 2 ▼ that the OK sensor also picks up from starting a grind.

While grinding, **touching any pad stops it** (one beep). The top line shows the dose, then progress, then the result (e.g. `Done 18.1 (+0.1)  14.3s`); the weight is in large digits below. Two beeps: the grind is done. Three: a safety stop.

**Menu** (hold ≡): Calibrate · Servo pos · Dose 1 · Dose 2 · Offset 1 · Offset 2 · Max time · Network.

- **Calibrate:** empty the scale, OK, put on a known weight (100–1000 g, in 100 g steps; a container of water weighed on a kitchen scale works), OK, check the reading, OK to save.
- **Servo pos:** set the rest position (just clear of the manual button) and the press position (holding it down). The grinder runs while the press position is held, so unplug it or keep a cup under it.
- **Dose 1 / Dose 2:** 1.0–99.9 g. Defaults 16.7 and 15.3 g (`include/config.h`).
- **Offset 1 / Offset 2:** how far short of the target the button is released, to allow for grounds still falling and the motor spinning down. Each dose has its own, so the two presets can use different beans that grind at different rates. It's learned: after each normal grind of that dose it moves halfway towards the miss, so it should settle within a few grinds. Set it by hand here if you like.
- **Max time:** the longest the button is ever held, in seconds (default 120). Raise it for a slow grinder or a big dose.
- **Network:** Wi-Fi and MQTT status, and the grind count.

### Safety stops

The servo lets go of the button if:
- the weight hasn't risen 0.3 g in 5 s (empty hopper, clog)
- the weight drops 5 g below its peak (cup lifted)
- the load cell stops sending readings for 1 s
- the button has been held for the max time (Menu → Max time, 120 s by default)

The SAMD21 watchdog also runs with a 2 s period whenever the servo isn't at rest: if the firmware hangs mid-grind, the board resets and the servo parks first thing on boot. The rest of the time it runs with a 16 s period, which only catches real lock-ups (a wedged Wi-Fi module, an unplugged HX711 mid-calibration). The limits are constants at the top of `include/grind.h`.

### After uploading new firmware

Uploading wipes the saved settings (FlashStorage lives in the program flash): calibration, servo positions, doses, the learned offsets, the max time and the grind count. The values it starts from after an upload are in `include/config.h`: calibration factor, servo positions, the two doses, max time, starting offset and the no-flow time.

To use your own values without editing that file, copy `include/config_local.example.h` to `include/config_local.h` and uncomment what you want to change. `config_local.h` is git-ignored, so your settings stay on your computer and don't clash with updates. Useful values to copy in: the calibration factor (printed over serial when a panel calibration is saved) and the servo positions (Menu → Servo pos shows them).

## Home Assistant (optional)

1. Copy `include/secrets.example.h` to `include/secrets.h` and fill in your 2.4 GHz Wi-Fi and MQTT broker (e.g. the Mosquitto add-on; use its IP address, since the Wi-Fi module can't resolve `.local` names). `secrets.h` is git-ignored.
2. Build and upload. A **GrinderBot** device appears in Home Assistant through MQTT discovery, with sensors for last dose, last target, last grind time, grind offset (for the dose last ground), last result and a grind count. Every grind is published retained to `grinderbot/<id>/grind`; grinds that reached the target also go to `grinderbot/<id>/dose`, which feeds the dose, target and time sensors, so stopped or cut-off grinds don't skew their statistics. The full record is in the attributes of *Last dose*.

Without `secrets.h` the firmware is built with no network code at all. With it, network calls only happen when the scale is idle (never while grinding), and a broker that's down just means retries every 15 s, backing off to 5 min. A connection attempt to a broker that doesn't answer can freeze the pads for up to ~11 s, so put in the right address. The grind count starts again from 0 after a firmware upload; Home Assistant treats that as a meter reset. If Wi-Fi connects but MQTT won't, update the Wi-Fi module firmware with the Arduino IDE's firmware updater (serial output shows the version at boot).

## Wiring

| Pin | |
|---|---|
| D0–D3 | Touch pads ≡, 1 ▲, 2 ▼, OK (TTP223, powered from 3.3 V) |
| D4 | Servo signal (servo power from 5V and GND) |
| D5 | Passive piezo buzzer |
| D6 / D7 | HX711 DOUT / SCK |
| D11 / D12 | OLED SDA / SCL (I²C 0x3C) |

## Tests

`test/host/run.sh` builds the pad, menu, settings and grind logic with a normal C++ compiler and runs it against a simulated scale and grinder (normal grinds, offset learning, every safety stop, menus, editors). `ci/build.yml` is a GitHub Actions workflow that runs those tests and builds the firmware with and without `secrets.h`; move it to `.github/workflows/` to run it on every push.
