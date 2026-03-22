#pragma once

#ifdef USE_RP2040

#include "esphome/core/component.h"
#include "esphome/core/automation.h"

#include <hardware/pio.h>

namespace esphome {
namespace beo_ir {

// Pre-compiled PIO program: counts 625us ticks between falling edges.
// Source: beo_pio.pio, compiled by pioasm 2.2.0
// Output: X register (counts down from 31). C++ converts: ticks = 32 - raw.
// Clock: sys_clk / 15625 = 8 kHz, 5-cycle tick loop = 625us per tick.

static const uint16_t beo_ir_pio_instructions[] = {
    0x20a0, //  0: wait   1 pin, 0
    0x2020, //  1: wait   0 pin, 0
    0xe03f, //  2: set    x, 31
    0xe040, //  3: set    y, 0
            //     .wrap_target
    0x01cb, //  4: jmp    pin, 11   [1]
    0x0069, //  5: jmp    !y, 9
    0xa0c1, //  6: mov    isr, x
    0x8000, //  7: push   noblock
    0x0002, //  8: jmp    2
    0x0144, //  9: jmp    x--, 4    [1]
    0x0000, // 10: jmp    0
    0xe141, // 11: set    y, 1      [1]
    0x0044, // 12: jmp    x--, 4
    0x0000, // 13: jmp    0
            //     .wrap
};

static const struct pio_program beo_ir_pio_program = {
    .instructions = beo_ir_pio_instructions,
    .length = 14,
    .origin = -1,
    .pio_version = 0,
};

// Symbol classification from tick counts (±2 tolerance)
enum BeoSymbol : uint8_t {
  SYM_ZERO,     // 5 ticks  (3-7)
  SYM_SAME,     // 10 ticks (8-12)
  SYM_ONE,      // 15 ticks (13-17)
  SYM_STOP,     // 20 ticks (18-22)
  SYM_START,    // 25 ticks (23-27)
  SYM_INVALID,
};

enum DecoderState : uint8_t {
  STATE_IDLE,
  STATE_DATA,
  STATE_EXPECT_STOP,
};

enum RepeatMode : uint8_t {
  REPEAT_RAW,
  REPEAT_TRANSLATE,
  REPEAT_SUPPRESS,
};

class BeoIRComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE; }

  void set_pin(uint8_t pin) { this->pin_ = pin; }
  void set_pio(int pio_num) { this->pio_ = pio_num ? pio1 : pio0; }
  void set_repeat_mode(RepeatMode mode) { this->repeat_mode_ = mode; }

  void add_on_command_callback(std::function<void(uint8_t, uint8_t, bool, bool)> &&cb) {
    this->command_callback_.add(std::move(cb));
  }

 protected:
  uint8_t pin_{15};
  PIO pio_{pio0};
  uint sm_{0};

  DecoderState state_{STATE_IDLE};
  uint8_t bit_count_{0};
  uint32_t shift_reg_{0};
  uint8_t prev_bit_{0};

  RepeatMode repeat_mode_{REPEAT_RAW};
  uint8_t last_address_{0};
  uint8_t last_command_{0};
  bool last_link_{false};
  uint32_t last_command_millis_{0};
  static const uint32_t REPEAT_WINDOW_MS = 300;

  CallbackManager<void(uint8_t, uint8_t, bool, bool)> command_callback_;

  static BeoSymbol classify_ticks_(uint32_t ticks);
  void decoder_reset_();
  bool process_symbol_(BeoSymbol sym, uint8_t &address, uint8_t &command, bool &link);
  void fire_command_(uint8_t address, uint8_t command, bool link);
};

// Trigger for on_command automation
class BeoCommandTrigger : public Trigger<uint8_t, uint8_t, bool, bool> {
 public:
  explicit BeoCommandTrigger(BeoIRComponent *parent) {
    parent->add_on_command_callback(
        [this](uint8_t address, uint8_t command, bool link, bool repeat) {
          this->trigger(address, command, link, repeat);
        });
  }
};

}  // namespace beo_ir
}  // namespace esphome

#endif  // USE_RP2040
