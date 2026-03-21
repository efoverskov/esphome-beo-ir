# esphome-beo-ir

Custom [ESPHome](https://esphome.io/) external component that decodes Bang & Olufsen Beo4/Beolink IR commands using the RP2040's PIO hardware. Decoded commands are published via Home Assistant for use in automations, Node-RED, or MQTT consumers.

Also supports the B&O IR eye's PCF8574T I2C I/O expander for physical button input and LED control using ESPHome's native `pcf8574` component.

Based on the [Beomote](https://github.com/christianlykke9/Beomote) Arduino library by Christian Lykke, re-implemented using RP2040 PIO hardware for more efficient IR decoding.

## How it works

The B&O IR protocol uses a 455 kHz carrier (demodulated by the IR eye into a digital signal). Symbols are encoded as gap durations measured in multiples of a 625 µs base tick. A frame consists of a START symbol, 1 link bit, 8 address bits, 8 command bits, and a STOP symbol.

The RP2040's PIO state machine counts ticks between falling edges and pushes the counts to a FIFO. The C++ component on the ARM core classifies tick counts into symbols and assembles complete frames. The PIO clock divider is computed dynamically from the actual system clock, so it works at both 125 MHz and 133 MHz.

## Requirements

### Hardware

- **Raspberry Pi Pico W** (RP2040)
- **B&O IR eye** — a passive [Beolink IR eye](https://beocentral.com/beolinkeye) receiver with a 5-pin connector.

  ![Beolink IR Eye](docs/beolink-eye.jpg)
- **3.3V/5V level shifter** — the IR eye outputs ~5V logic, but RP2040 GPIOs are rated for max 3.63V. Level shifting is required on:
  - IR data line
  - I2C SDA and SCL (if using the eye's buttons/LEDs)

  A [4-channel I2C-safe bidirectional level shifter](https://www.aliexpress.com/item/1005007050973483.html) works well — it handles both the I2C lines and the IR data signal with a single board.

### IR eye connector pinout

![IR eye pinout](docs/ir-eye-pinout.png)

| Pin | Signal  |
|-----|---------|
| 1   | SCL     |
| 2   | SDA     |
| 3   | IR data |
| 4   | 5V      |
| 5   | GND     |

### Software

- [ESPHome](https://esphome.io/) 2024.1.0 or later (RP2040 platform support). Tested on 2026.3.0
- [Home Assistant](https://www.home-assistant.io/) with the ESPHome integration

### Default GPIO assignments

| Function | GPIO |
|----------|------|
| IR input | GP15 |
| I2C SDA  | GP4  |
| I2C SCL  | GP5  |

## Installation

1. Copy the `components/beo_ir/` directory into your ESPHome config's `components/` folder.

2. Add the component to your device YAML:

```yaml
external_components:
  - source:
      type: local
      path: components

beo_ir:
  pin: 15
  pio: 0
  on_command:
    - then:
        - logger.log:
            format: "B&O: addr=0x%02X(%s) cmd=0x%02X(%s) link=%d"
            args:
              - address
              - beo_address_name(address)
              - command
              - beo_command_name(command)
              - int(link)
```

3. Compile and flash from the ESPHome dashboard.

See [example.yaml](example.yaml) for a complete configuration including MQTT publishing and IR eye button/LED setup.

## Configuration

### `beo_ir` component

| Option       | Required | Default | Description                                    |
|--------------|----------|---------|------------------------------------------------|
| `pin`        | Yes      | —       | GPIO pin connected to the IR data line (0–29)  |
| `pio`        | No       | `0`     | PIO instance to use (0 or 1)                   |
| `on_command` | No       | —       | Automation trigger for decoded commands        |

### `on_command` trigger variables

| Variable  | Type      | Description                                   |
|-----------|-----------|-----------------------------------------------|
| `address` | `uint8_t` | B&O device address (e.g., 0x01 for AUDIO)     |
| `command` | `uint8_t` | B&O command code (e.g., 0x60 for VOLUME_UP)   |
| `link`    | `bool`    | True if the link bit was set                  |

Helper functions `beo_address_name(address)` and `beo_command_name(command)` return human-readable names for known codes.

## MQTT publishing

The ESPHome `mqtt:` component is not supported on RP2040. The workaround is to use the native API (`api:`) and call Home Assistant's `mqtt.publish` service:

```yaml
- homeassistant.service:
    service: mqtt.publish
    data:
      topic: beo2mqtt/command
      payload: !lambda |-
        char buf[256];
        snprintf(buf, sizeof(buf),
          "{\"BeoSource\":\"ir\",\"Beolink\":%d,"
          "\"BeoAddress\":%d,\"BeoAddressName\":\"%s\","
          "\"BeoCommand\":%d,\"BeoCommandName\":\"%s\"}",
          link ? 1 : 0,
          address, beo_address_name(address),
          command, beo_command_name(command));
        return std::string(buf);
```

## IR eye buttons and LEDs

The IR eye's PCF8574T I/O expander is supported via ESPHome's native `pcf8574` component — no custom code needed. I2C must run at 10 kHz for reliable communication through the level shifter.

| PCF8574T Pin | Direction | Function                  |
|--------------|-----------|---------------------------|
| P0           | Input     | Timer button              |
| P1           | Input     | Volume Up button          |
| P2           | Input     | Volume Down button        |
| P3           | Input     | Play button               |
| P4–P5        | —         | Unused                    |
| P6           | Output    | Standby LED (active low)  |
| P7           | Output    | Timer LED (active low)    |

See [example.yaml](example.yaml) for the full I2C/button/LED configuration.

## License

MIT
