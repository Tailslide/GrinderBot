# GrinderBot

A scale that sits under a Mazzer Mini Electronic (Type A) and grinds by weight: a servo presses and holds the grinder's manual button, and lets go when the cup reaches the dose. Arduino MKR WiFi 1010, HX711 + 1 kg load cell, 128×32 OLED, four touch pads, a piezo buzzer and an MG90S servo. PlatformIO project.

## Using it

| Pad | On the weight screen | In menus and settings |
|---|---|---|
| **OK** | Grind the selected dose (put the cup on first; it tares) | Select / save |
| **1 ▲** | Tap: dose 1. Hold 2 s: edit dose 1 | Up / more |
| **2 ▼** | Tap: dose 2. Hold 2 s: edit dose 2 | Down / less |
| **≡** | Tap: tare. Hold 2 s: menu | Back / cancel |

While grinding, **touching any pad stops it**. The top line shows the dose, then progress, then the result (e.g. `Done 18.1 (+0.1)  14.3s`); the weight is in large digits below. Two beeps mean a grind finished or was stopped; three mean a safety stop.

**Menu** (hold ≡): Calibrate · Servo pos · Dose 1 · Dose 2 · Offset · Network.

- **Calibrate:** empty the scale, OK, put on a known weight (100–1000 g, in 100 g steps; a container of water weighed on a kitchen scale works), OK, check the reading, OK to save.
- **Servo pos:** set the rest position (just clear of the manual button) and the press position (holding it down). The grinder runs while the press position is held, so unplug it or keep a cup under it.
- **Dose 1 / Dose 2:** 1.0–99.9 g. Defaults 9.0 and 18.0 g.
- **Offset:** how far short of the target the button is released, to allow for grounds still falling and the motor spinning down. It's learned: after each normal grind it moves halfway towards the miss, so it should settle within a few grinds. Set it by hand here if you like.
- **Network:** Wi-Fi and MQTT status, and the grind count.

### Safety stops

The servo lets go of the button if:
- the weight hasn't risen 0.3 g in 5 s (empty hopper, clog)
- the weight drops 5 g below its peak (cup lifted)
- the load cell stops sending readings for 1 s
- the button has been held 60 s

The SAMD21 watchdog also runs whenever the servo isn't at rest. If the firmware hangs mid-grind, the board resets within 2 s and the servo parks first thing on boot. The limits are constants at the top of `include/grind.h`.

### After uploading new firmware

Uploading wipes the saved settings (FlashStorage lives in the program flash): calibration, servo positions, doses and the learned offset. Put your calibration factor in `DEFAULT_CAL_FACTOR` (`include/loadcell.h`; the panel calibration prints it over serial) so readings stay in grams. Set the servo positions again from the menu.

## Home Assistant (optional)

1. Copy `include/secrets.example.h` to `include/secrets.h` and fill in your 2.4 GHz Wi-Fi and MQTT broker (e.g. the Mosquitto add-on; use its IP address, since the Wi-Fi module can't resolve `.local` names). `secrets.h` is git-ignored.
2. Build and upload. A **GrinderBot** device appears in Home Assistant through MQTT discovery, with sensors for last dose, last target, last grind time, grind offset, last result and a grind count. The full record of each grind is in the attributes of *Last dose*, and is published retained to `grinderbot/<id>/grind`.

Without `secrets.h` the firmware is built with no network code at all. With it, network calls only happen when the scale is idle (never while grinding), and a broker that's down just means retries every 15 s up to 5 min. If Wi-Fi connects but MQTT won't, update the Wi-Fi module firmware with the Arduino IDE's firmware updater (serial output shows the version at boot).

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
