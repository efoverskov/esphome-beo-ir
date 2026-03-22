# Implementation Guide — esphome-beo-ir

Custom ESPHome component for decoding Bang & Olufsen IR commands on the Raspberry Pi Pico W using PIO hardware. Everything except the IR decoder is standard ESPHome YAML config.

---

## Phase 1: B&O IR Decoder (Custom PIO Component) — COMPLETE

**Goal:** Decode B&O IR frames using a PIO state machine and fire ESPHome automations.

**Status:** Working on hardware. Verified decoding of VOLUME_UP, VOLUME_DOWN, YELLOW, GREEN, NAV commands and others.

### B&O IR Protocol

The B&O IR protocol uses a 455 kHz carrier (demodulated by the IR eye). The demodulated signal encodes data as variable-length gaps between falling edges, measured in 625 µs ticks:

| Symbol | Ticks | Duration | Meaning |
|--------|-------|----------|---------|
| ZERO   | 5     | 3,125 µs | Bit value 0 |
| SAME   | 10    | 6,250 µs | Repeat previous bit value |
| ONE    | 15    | 9,375 µs | Bit value 1 |
| STOP   | 20    | 12,500 µs | End of frame |
| START  | 25    | 15,625 µs | Start of frame |

Tolerance: ±1 tick on each symbol (matching original Beomote Arduino library).

Frame structure: START → 1 link bit → 8 address bits → 8 command bits → STOP = 19 symbols total.

### PIO Hybrid Approach

PIO handles only the timing-critical part (counting 625 µs ticks between falling edges). The C++ code on the ARM core classifies symbols and assembles frames. Symbols arrive every 3–25 ms so there's no time pressure on the CPU side.

PIO program details:
- Clock divider: computed dynamically from `clock_get_hz(clk_sys)` to target 8 kHz
- 5-cycle tick loop → 625 µs per tick (exact match to Beomote reference)
- X register counts down from 31; C++ converts: `ticks = 32 - raw`
- Timeout after 31 ticks re-syncs from scratch

### Address Validation

Only frames with known B&O addresses are accepted. Frames with unknown addresses are logged and dropped to eliminate false positives from noise.

### Component Files

```
components/beo_ir/
├── __init__.py       # ESPHome component registration + config schema
├── beo_ir.h          # BeoIRComponent class + pre-compiled PIO instructions
├── beo_ir.cpp        # PIO setup, frame decoder state machine
└── beo_commands.h    # Address/command enums + name lookup tables
```

### Phase 1 Test — PASSED

Pressed Beo4 buttons, verified decoded output in ESPHome logs:
```
[D][beo_ir:062]: B&O: link=0 addr=0x01(AUDIO) cmd=0xD4(YELLOW)
[D][beo_ir:062]: B&O: link=0 addr=0x01(AUDIO) cmd=0xD5(GREEN)
```

---

## Phase 2: Home Assistant Integration — COMPLETE

**Goal:** Expose B&O commands to Home Assistant for use in automations.

**Status:** Working. Uses native API with `homeassistant.service` to call `mqtt.publish`.

**Note:** The ESPHome `mqtt:` component is not supported on RP2040 (as of 2026.3.0). The workaround uses the native API (`api:`) and calls HA's `mqtt.publish` service via `homeassistant.service`. This works reliably.

### MQTT JSON Payload

```json
{
  "BeoSource": "ir",
  "Beolink": 0,
  "BeoAddress": 1,
  "BeoAddressName": "AUDIO",
  "BeoCommand": 212,
  "BeoCommandName": "YELLOW"
}
```

See `example.yaml` for the full working config.

---

## Phase 3: I2C IR Eye — Button & LED Control (PCF8574T) — COMPLETE

**Goal:** Interface with the PCF8574T I/O expander on the B&O IR eye to read buttons and control LEDs.

**Status:** Pin mapping discovered, buttons and LEDs working.

Uses ESPHome's native `pcf8574` component — no custom driver code needed. See `docs/I2C_NOTES.md` for the discovered pin mapping and `example.yaml` for the full config.

### Discovered Pin Map

| Pin | Function | Direction |
|-----|----------|-----------|
| P0 | Timer button | Input |
| P1 | Volume Up button | Input |
| P2 | Volume Down button | Input |
| P3 | Play button | Input |
| P4 | (unused) | — |
| P5 | (unused) | — |
| P6 | Standby LED | Output |
| P7 | Timer LED | Output |

Eye buttons publish to the same `beo2mqtt/command` MQTT topic with `"BeoSource": "eye"`.

---

## Hardware Notes

- B&O IR eye data pin outputs ~5V logic. RP2040 GPIOs max 3.63V. **Requires level shifter** on IR data, SDA, and SCL lines.
- IR eye connector: Pin 1=SCL, Pin 2=SDA, Pin 3=IR data, Pin 4=5V, Pin 5=GND
- Default GPIO: IR input on GP15, I2C on GP4 (SDA) / GP5 (SCL)
- I2C frequency: 10 kHz (required for reliable comms through level shifter)
- PCF8574T address: 0x20
