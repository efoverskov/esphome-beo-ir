# Implementation Guide — esphome-beo-ir

Custom ESPHome component for decoding Bang & Olufsen IR commands on the Raspberry Pi Pico W using PIO hardware. Everything except the IR decoder is standard ESPHome YAML config.

---

## Phase 1: B&O IR Decoder (Custom PIO Component) — COMPLETE

**Goal:** Decode B&O IR frames using a PIO state machine and fire ESPHome automations.

**Status:** Working on hardware. Verified decoding of VOLUME_UP, VOLUME_DOWN, YELLOW, GREEN, NAV_UP/DOWN/LEFT/RIGHT commands and others.

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

**Goal:** Interface with the PCF8574T I/O expander on the B&O IR eye to read buttons and control LEDs, with repeat/hold handling matching the IR remote behaviour.

**Status:** Pin mapping discovered, buttons and LEDs working. Eye buttons route through the same `fire_command_()` path as IR commands and obey the configured `repeat_mode`.

LEDs use ESPHome's native `pcf8574` component — no custom driver code needed. Buttons are configured as `eye_buttons` in the `beo_ir` component, which polls the PCF8574T pins, debounces input, and generates repeat events for held buttons.

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

### Eye Button Repeat Handling

Eye buttons are configured in the `beo_ir` component with per-button `repeat` enable/disable:

```yaml
eye_buttons:
  - pin: { pcf8574: ir_eye_io, number: 1, mode: INPUT, inverted: true }
    command: 0x60  # VOLUME_UP
    repeat: true   # generate repeats when held
  - pin: { pcf8574: ir_eye_io, number: 3, mode: INPUT, inverted: true }
    command: 0x35  # GO (Play)
    repeat: false  # single press only
```

Timing: 50 ms debounce, 400 ms initial delay before repeats, then 200 ms repeat interval. Repeats fire through the same `fire_command_()` path as IR commands and obey the configured `repeat_mode` (raw/translate/suppress).

The `on_command` trigger includes a `source` variable (`"ir"` or `"eye"`) so automations can distinguish the origin. Eye buttons publish to the same MQTT topic with `"BeoSource": "eye"`.

See `docs/I2C_NOTES.md` for the discovered pin mapping and `example.yaml` for the full config.

---

## Phase 4: Repeat/Hold Handling — COMPLETE

**Goal:** Properly handle held-down buttons, which the Beo4 signals using repeat codes.

**Status:** Implemented. Three repeat modes (raw/translate/suppress) with optional HA Select entity for runtime switching.

### How B&O repeats work

The Beo4 remote uses three different mechanisms to signal a held button:

**1. Button-specific repeat codes:** Some buttons have a dedicated repeat command code that differs from the initial press. The remote sends the pilot code once, then switches to the repeat code for subsequent frames.

| Pilot code | Repeat code |
|------------|-------------|
| UP (0x1E) | UP_REPEAT (0x72) |
| DOWN (0x1F) | DOWN_REPEAT (0x73) |
| LEFT (0x32) | LEFT_REPEAT (0x70) |
| RIGHT (0x34) | RIGHT_REPEAT (0x71) |
| VOLUME_UP (0x60) | VOLUME_UP_REPEAT (0xB4) |
| VOLUME_DOWN (0x64) | VOLUME_DOWN_REPEAT (0xB8) |
| GREEN (0xD5) | GREEN_REPEAT (0x76) |
| YELLOW (0xD4) | YELLOW_REPEAT (0x77) |
| BLUE (0xD8) | BLUE_REPEAT (0x78) |
| RED (0xD9) | RED_REPEAT (0x79) |

**2. Repeated identical frames:** Some buttons simply re-send the same command code. For example, holding VOLUME_UP sends `VOLUME_UP → VOLUME_UP → VOLUME_UP...`. The component must use timing to distinguish a held button from rapid distinct presses.

**3. Generic REPEAT code (0x75):** A single code meaning "repeat whatever was last sent". Used by buttons that don't have their own repeat variant. The component must track the previous command to resolve what REPEAT refers to.

### Configurable behaviour

Add a `repeat_mode` option to the YAML config:

```yaml
beo_ir:
  pin: 15
  pio: 0
  repeat_mode: translate  # raw | translate | suppress (default: raw)
```

| Mode | Behaviour |
|------|-----------|
| `raw` | Pass all codes through as-is. Automation sees `YELLOW_REPEAT`, `REPEAT`, duplicate `VOLUME_UP` frames, etc. The `repeat` trigger variable is always `false`. |
| `translate` | Normalize all three repeat mechanisms into repeated original commands with `repeat: true`. E.g. `YELLOW_REPEAT` → `command=YELLOW, repeat=true`. `REPEAT` (0x75) → resolves to previous command with `repeat=true`. Duplicate identical frames within the repeat window → same command with `repeat=true`. |
| `suppress` | Only fire `on_command` for the initial pilot press. All repeats (of any kind) are dropped. |

In `translate` and `suppress` modes, the `on_command` trigger gains a `repeat` (bool) variable.

### Implementation

- Add a lookup table mapping button-specific repeat codes → pilot codes
- Track last command + timestamp to:
  - Resolve the generic REPEAT code (0x75) to the previous command
  - Detect repeated identical frames (mechanism 2) within a timing window
- Add `repeat_mode` config option to `__init__.py` and pass to C++
- Trigger variables: `(uint8_t address, uint8_t command, bool link, bool repeat, std::string source)`

### Phase 4 Test

- Hold YELLOW → in `raw` mode, verify `YELLOW` then `YELLOW_REPEAT` frames appear
- Hold YELLOW → in `translate` mode, verify repeated `YELLOW` frames with `repeat=true`
- Hold YELLOW → in `suppress` mode, verify only one `YELLOW` event fires
- Hold VOLUME_UP → verify repeated `VOLUME_UP` frames are detected via timing
- Test a button that uses generic REPEAT (0x75) → verify it resolves to the previous command

---

## Hardware Notes

- B&O IR eye data pin outputs ~5V logic. RP2040 GPIOs max 3.63V. **Requires level shifter** on IR data, SDA, and SCL lines.
- IR eye connector: Pin 1=SCL, Pin 2=SDA, Pin 3=IR data, Pin 4=5V, Pin 5=GND
- Default GPIO: IR input on GP15, I2C on GP4 (SDA) / GP5 (SCL)
- I2C frequency: 10 kHz (required for reliable comms through level shifter)
- PCF8574T address: 0x20
