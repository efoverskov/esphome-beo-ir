#pragma once

#ifdef USE_RP2040

#include "beo_ir.h"
#include "esphome/components/select/select.h"

namespace esphome {
namespace beo_ir {

class BeoRepeatModeSelect : public select::Select {
 public:
  void set_beo_ir(BeoIRComponent *parent) { this->parent_ = parent; }

 protected:
  void control(const std::string &value) override {
    if (value == "raw") {
      this->parent_->set_repeat_mode(REPEAT_RAW);
    } else if (value == "translate") {
      this->parent_->set_repeat_mode(REPEAT_TRANSLATE);
    } else if (value == "suppress") {
      this->parent_->set_repeat_mode(REPEAT_SUPPRESS);
    }
    this->publish_state(value);
  }

  BeoIRComponent *parent_{nullptr};
};

}  // namespace beo_ir
}  // namespace esphome

#endif  // USE_RP2040
