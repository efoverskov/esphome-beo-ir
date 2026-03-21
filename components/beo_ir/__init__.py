"""B&O IR Receiver component for ESPHome (RP2040 PIO)."""

from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PIN, CONF_TRIGGER_ID

DEPENDENCIES = ["rp2040"]
CODEOWNERS = ["@esf"]

beo_ir_ns = cg.esphome_ns.namespace("beo_ir")
BeoIRComponent = beo_ir_ns.class_("BeoIRComponent", cg.Component)

CONF_ON_COMMAND = "on_command"
CONF_PIO = "pio"

BeoCommandTrigger = beo_ir_ns.class_(
    "BeoCommandTrigger",
    automation.Trigger.template(cg.uint8, cg.uint8, cg.bool_),
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BeoIRComponent),
        cv.Required(CONF_PIN): cv.int_range(min=0, max=29),
        cv.Optional(CONF_PIO, default=0): cv.one_of(0, 1, int=True),
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

    for conf in config.get(CONF_ON_COMMAND, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(
            trigger,
            [(cg.uint8, "address"), (cg.uint8, "command"), (cg.bool_, "link")],
            conf,
        )
