"""B&O IR Receiver component for ESPHome (RP2040 PIO)."""

from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN, CONF_TRIGGER_ID
from esphome.components import select as select_comp

DEPENDENCIES = ["rp2040"]
AUTO_LOAD = ["select"]
CODEOWNERS = ["@esf"]

beo_ir_ns = cg.esphome_ns.namespace("beo_ir")
BeoIRComponent = beo_ir_ns.class_("BeoIRComponent", cg.Component)
BeoRepeatModeSelect = beo_ir_ns.class_(
    "BeoRepeatModeSelect", select_comp.Select
)

CONF_ON_COMMAND = "on_command"
CONF_PIO = "pio"
CONF_REPEAT_MODE = "repeat_mode"
CONF_REPEAT_MODE_SELECT = "repeat_mode_select"

REPEAT_MODE_OPTIONS = ["raw", "translate", "suppress"]

RepeatMode = beo_ir_ns.enum("RepeatMode")
REPEAT_MODES = {
    "raw": RepeatMode.REPEAT_RAW,
    "translate": RepeatMode.REPEAT_TRANSLATE,
    "suppress": RepeatMode.REPEAT_SUPPRESS,
}

BeoCommandTrigger = beo_ir_ns.class_(
    "BeoCommandTrigger",
    automation.Trigger.template(cg.uint8, cg.uint8, cg.bool_, cg.bool_),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BeoIRComponent),
        cv.Required(CONF_PIN): cv.int_range(min=0, max=29),
        cv.Optional(CONF_PIO, default=0): cv.one_of(0, 1, int=True),
        cv.Optional(CONF_REPEAT_MODE, default="raw"): cv.enum(REPEAT_MODES, lower=True),
        cv.Optional(CONF_REPEAT_MODE_SELECT): select_comp.select_schema(
            BeoRepeatModeSelect,
        ),
        cv.Optional(CONF_ON_COMMAND): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BeoCommandTrigger),
            }
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_pin(config[CONF_PIN]))
    cg.add(var.set_pio(config[CONF_PIO]))
    cg.add(var.set_repeat_mode(config[CONF_REPEAT_MODE]))

    if CONF_REPEAT_MODE_SELECT in config:
        sel = await select_comp.new_select(
            config[CONF_REPEAT_MODE_SELECT],
            options=REPEAT_MODE_OPTIONS,
        )
        cg.add(sel.set_beo_ir(var))
        # Set initial state matching the YAML default
        for name, val in REPEAT_MODES.items():
            if val == config[CONF_REPEAT_MODE]:
                cg.add(sel.publish_state(name))
                break

    for conf in config.get(CONF_ON_COMMAND, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger,
            [(cg.uint8, "address"), (cg.uint8, "command"), (cg.bool_, "link"), (cg.bool_, "repeat")],
            conf,
        )
