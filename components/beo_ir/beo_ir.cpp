#ifdef USE_RP2040

#include "beo_ir.h"
#include "beo_commands.h"
#include "esphome/core/log.h"

#include <hardware/pio.h>
#include <hardware/gpio.h>
#include <hardware/clocks.h>

namespace esphome {
namespace beo_ir {

static const char *TAG = "beo_ir";

// Number of data bits: 1 link + 8 address + 8 command
static const uint8_t FRAME_DATA_BITS = 17;

void BeoIRComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up B&O IR Receiver...");

  uint offset = pio_add_program(this->pio_, &beo_ir_pio_program);
  this->sm_ = pio_claim_unused_sm(this->pio_, true);

  // Configure PIO state machine
  pio_sm_config c = pio_get_default_sm_config();
  sm_config_set_wrap(&c, offset + 4, offset + 13);  // wrap_target=4, wrap=13

  sm_config_set_in_pins(&c, this->pin_);
  sm_config_set_jmp_pin(&c, this->pin_);

  pio_gpio_init(this->pio_, this->pin_);
  pio_sm_set_pindirs_with_mask(this->pio_, this->sm_, 0, 1u << this->pin_);
  gpio_pull_up(this->pin_);

  // Compute divider dynamically: target 8 kHz PIO clock → 625us per tick
  // (5-cycle tick loop × 125us per PIO cycle = 625us)
  float clkdiv = (float)clock_get_hz(clk_sys) / 8000.0f;
  ESP_LOGCONFIG(TAG, "  System clock: %lu Hz, PIO divider: %.1f", clock_get_hz(clk_sys), clkdiv);
  sm_config_set_clkdiv(&c, clkdiv);
  sm_config_set_in_shift(&c, false, false, 32);

  pio_sm_init(this->pio_, this->sm_, offset, &c);
  pio_sm_set_enabled(this->pio_, this->sm_, true);

  this->decoder_reset_();

  for (auto &btn : this->eye_buttons_) {
    btn.pin->setup();
    btn.raw_state = btn.pin->digital_read();
    btn.pressed = false;
    btn.state_change_millis = millis();
  }

  ESP_LOGCONFIG(TAG, "B&O IR Receiver ready on GPIO%d (PIO%d SM%d), %d eye button(s)",
                this->pin_, this->pio_ == pio0 ? 0 : 1, this->sm_,
                (int) this->eye_buttons_.size());
}

void BeoIRComponent::loop() {
  while (!pio_sm_is_rx_fifo_empty(this->pio_, this->sm_)) {
    uint32_t raw = pio_sm_get(this->pio_, this->sm_);
    uint32_t ticks = 32 - raw;

    BeoSymbol sym = classify_ticks_(ticks);
    if (sym == SYM_INVALID) {
      this->decoder_reset_();
      continue;
    }

    uint8_t address, command;
    bool link;
    if (this->process_symbol_(sym, address, command, link)) {
      if (!is_valid_beo_address(address)) {
        ESP_LOGD(TAG, "B&O: dropped frame with unknown addr=0x%02X cmd=0x%02X", address, command);
        continue;
      }
      ESP_LOGD(TAG, "B&O: link=%d addr=0x%02X(%s) cmd=0x%02X(%s)",
               link, address, beo_address_name(address),
               command, beo_command_name(command));
      this->fire_command_(address, command, link, "ir");
    }
  }

  // Poll eye buttons
  uint32_t now = millis();
  for (auto &btn : this->eye_buttons_) {
    bool raw = btn.pin->digital_read();

    // Debounce: track when raw state last changed
    if (raw != btn.raw_state) {
      btn.raw_state = raw;
      btn.state_change_millis = now;
    }

    uint32_t stable_for = now - btn.state_change_millis;
    if (stable_for >= EYE_DEBOUNCE_MS && raw != btn.pressed) {
      btn.pressed = raw;
      if (btn.pressed) {
        // Initial press
        btn.press_millis = now;
        btn.last_repeat_millis = now;
        ESP_LOGD(TAG, "Eye: %s pressed", beo_command_name(btn.command));
        this->fire_command_(btn.address, btn.command, false, "eye");
      }
    }

    // Generate repeats while held
    if (btn.pressed && btn.repeat_enabled) {
      uint32_t since_press = now - btn.press_millis;
      uint32_t since_last = now - btn.last_repeat_millis;
      if (since_press >= EYE_REPEAT_INITIAL_MS && since_last >= EYE_REPEAT_INTERVAL_MS) {
        btn.last_repeat_millis = now;
        this->fire_command_(btn.address, btn.command, false, "eye", true);
      }
    }
  }
}

BeoSymbol BeoIRComponent::classify_ticks_(uint32_t ticks) {
  if (ticks >=  4 && ticks <=  6) return SYM_ZERO;
  if (ticks >=  9 && ticks <= 11) return SYM_SAME;
  if (ticks >= 14 && ticks <= 16) return SYM_ONE;
  if (ticks >= 19 && ticks <= 21) return SYM_STOP;
  if (ticks >= 24 && ticks <= 26) return SYM_START;
  return SYM_INVALID;
}

void BeoIRComponent::decoder_reset_() {
  this->state_ = STATE_IDLE;
  this->bit_count_ = 0;
  this->shift_reg_ = 0;
  this->prev_bit_ = 0;
}

bool BeoIRComponent::process_symbol_(BeoSymbol sym, uint8_t &address,
                                     uint8_t &command, bool &link) {
  switch (this->state_) {
    case STATE_IDLE:
      if (sym == SYM_START) {
        this->state_ = STATE_DATA;
        this->bit_count_ = 0;
        this->shift_reg_ = 0;
        this->prev_bit_ = 0;
      }
      break;

    case STATE_DATA: {
      uint8_t bit_val;
      if (sym == SYM_ZERO) {
        bit_val = 0;
      } else if (sym == SYM_ONE) {
        bit_val = 1;
      } else if (sym == SYM_SAME) {
        bit_val = this->prev_bit_;
      } else {
        this->decoder_reset_();
        break;
      }

      this->shift_reg_ = (this->shift_reg_ << 1) | bit_val;
      this->prev_bit_ = bit_val;
      this->bit_count_++;

      if (this->bit_count_ >= FRAME_DATA_BITS)
        this->state_ = STATE_EXPECT_STOP;
      break;
    }

    case STATE_EXPECT_STOP:
      if (sym == SYM_STOP) {
        command = this->shift_reg_ & 0xFF;
        address = (this->shift_reg_ >> 8) & 0xFF;
        link = (this->shift_reg_ >> 16) & 0x01;
        this->decoder_reset_();
        return true;
      }
      this->decoder_reset_();
      break;
  }
  return false;
}

void BeoIRComponent::fire_command_(uint8_t address, uint8_t command, bool link,
                                    const std::string &source, bool known_repeat) {
  uint32_t now = millis();

  if (this->repeat_mode_ == REPEAT_RAW) {
    this->last_address_ = address;
    this->last_command_ = command;
    this->last_link_ = link;
    this->last_command_millis_ = now;
    this->command_callback_.call(address, command, link, false, source);
    return;
  }

  // translate / suppress mode: detect and resolve repeats
  bool is_repeat = known_repeat;
  uint8_t resolved_command = command;

  if (!known_repeat) {
    // Mechanism 1: button-specific repeat code (e.g. YELLOW_REPEAT → YELLOW)
    uint8_t pilot = beo_repeat_to_pilot(command);
    if (pilot != 0xFF) {
      resolved_command = pilot;
      is_repeat = true;
    }
    // Mechanism 3: generic REPEAT (0x75) → resolve to last command
    else if (command == BEO_CMD_REPEAT) {
      if (this->last_command_millis_ != 0 &&
          (now - this->last_command_millis_) < REPEAT_WINDOW_MS) {
        resolved_command = this->last_command_;
        address = this->last_address_;
        link = this->last_link_;
        is_repeat = true;
      } else {
        ESP_LOGD(TAG, "B&O: REPEAT with no recent command, dropping");
        return;
      }
    }
    // Mechanism 2: repeated identical frame within timing window
    else if (this->last_command_millis_ != 0 &&
             address == this->last_address_ &&
             command == this->last_command_ &&
             (now - this->last_command_millis_) < REPEAT_WINDOW_MS) {
      is_repeat = true;
    }
  }

  this->last_address_ = address;
  this->last_command_ = resolved_command;
  this->last_link_ = link;
  this->last_command_millis_ = now;

  if (this->repeat_mode_ == REPEAT_SUPPRESS && is_repeat) {
    ESP_LOGD(TAG, "B&O: suppressed repeat cmd=0x%02X(%s)",
             resolved_command, beo_command_name(resolved_command));
    return;
  }

  this->command_callback_.call(address, resolved_command, link, is_repeat, source);
}

void BeoIRComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "B&O IR Receiver:");
  ESP_LOGCONFIG(TAG, "  Pin: GPIO%d", this->pin_);
  ESP_LOGCONFIG(TAG, "  PIO: %d", this->pio_ == pio0 ? 0 : 1);
  const char *mode = "raw";
  if (this->repeat_mode_ == REPEAT_TRANSLATE) mode = "translate";
  else if (this->repeat_mode_ == REPEAT_SUPPRESS) mode = "suppress";
  ESP_LOGCONFIG(TAG, "  Repeat mode: %s", mode);
  for (auto &btn : this->eye_buttons_) {
    ESP_LOGCONFIG(TAG, "  Eye button: cmd=0x%02X(%s) addr=0x%02X repeat=%s",
                  btn.command, beo_command_name(btn.command),
                  btn.address, btn.repeat_enabled ? "yes" : "no");
  }
}

}  // namespace beo_ir
}  // namespace esphome

#endif  // USE_RP2040
