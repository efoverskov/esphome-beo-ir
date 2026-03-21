# B&O IR Eye — I2C Protocol Notes

## Status: PCF8574T CONFIRMED — pin mapping complete

The IR eye uses an **NXP PCF8574T** 8-bit I/O expander. This is a standard, well-documented part. No proprietary protocol reverse-engineering needed.

---

## PCF8574T Overview

- **Datasheet:** NXP PCF8574/PCF8574A
- **I2C address:** 0x20–0x27 (PCF8574T), set by A0/A1/A2 hardware pins
- **I/O:** 8 quasi-bidirectional pins (P0–P7)
- **Protocol:** No register addressing. Write 1 byte → sets all 8 outputs. Read 1 byte → gets all 8 input states.
- **INT pin:** Open-drain, active-low. Triggers on any input pin change. Cleared by reading the port.

### How it works

**Writing (LED control):**
```
I2C START → [addr|W] → [data_byte] → STOP
```
Each bit in data_byte controls one pin. For active-low LEDs: 0 = LED on, 1 = LED off.

**Reading (button state):**
```
I2C START → [addr|R] → [data_byte] → STOP
```
Each bit reflects pin state. For active-low buttons: 0 = pressed, 1 = released.

**Important:** Pins used as inputs must be written HIGH first (write 1 to those bits). The quasi-bidirectional output can only weakly pull high (~100µA), so an external button pulling to GND will override it. Pins driven low as outputs sink up to 25mA (enough for LEDs).

### Interrupt (INT pin)

The PCF8574T has an INT output that goes low when any input changes from the last read value. This avoids polling:
1. Configure a Pico GPIO as input with interrupt on falling edge
2. On interrupt → read the PCF8574T → get new button state
3. Reading clears the interrupt

Check if the IR eye's connector exposes the INT line, or if it's only SDA/SCL/VCC/GND. If INT is not exposed, polling at ~50ms intervals works fine.

---

## Pin Mapping (TO BE DETERMINED)

Connect to the IR eye and determine which P0–P7 pins are buttons vs LEDs.

### Discovery procedure

1. I2C scan to confirm address (expect 0x20–0x27)
2. Read the port with no buttons pressed → baseline (button pins will read 1, LED pins state depends on init)
3. Press each button one at a time, read after each → identifies button pin numbers
4. Write 0x00 (all low) → see which LEDs turn on
5. Write 0xFF (all high) → see which LEDs turn off
6. Toggle individual bits to map each LED

### Pin Map (fill in during testing)

| Bit | Pin | Direction | Physical Label | B&O Address | B&O Command | Notes |
|-----|-----|-----------|---------------|-------------|-------------|-------|
| 0 | P0 | Input | Timer | AUDIO (0x01) | EYE_TIMER (0xF0) | Custom code, not in Beo4 protocol |
| 1 | P1 | Input | Volume Up | AUDIO (0x01) | VOLUME_UP (0x60) | |
| 2 | P2 | Input | Volume Down | AUDIO (0x01) | VOLUME_DOWN (0x64) | |
| 3 | P3 | Input | Play | AUDIO (0x01) | PLAY (0x35) | |
| 4 | P4 | — | (unused) | | | Not connected |
| 5 | P5 | — | (unused) | | | Not connected |
| 6 | P6 | Output | Standby LED | | | Active low |
| 7 | P7 | Output | Timer LED | | | Active low |

For button pins, the B&O Address + Command columns map the physical button to the
equivalent Beo4 remote command. For example, a volume-up button would map to
`BEO_ADDR_AUDIO` (0x01) + `BEO_CMD_VOLUME_UP` (0x60). This mapping is used by the
`i2c_eye` driver to produce `beo_frame_t` structs identical to IR-decoded commands.

### IR Eye I2C Address

- Confirmed address: 0x20
- Discovery method: ESPHome `i2c: scan: true` + `pcf8574` component
- I2C frequency: 10 kHz (required for reliable comms through level shifter)

---

## ESPHome Integration

No custom driver needed — ESPHome's native `pcf8574` component handles everything. Buttons are `binary_sensor` entities with `on_press` automations, LEDs are `switch` entities. See `example.yaml` for the full config.
