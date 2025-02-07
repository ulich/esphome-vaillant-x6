import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor, uart
from esphome.const import *
vaillant_x6_ns = cg.esphome_ns.namespace('vaillant_x6')
VaillantX6Component = vaillant_x6_ns.class_('VaillantX6Component', cg.PollingComponent)

DEPENDENCIES = ['uart']
AUTO_LOAD = ['uart', 'sensor', 'binary_sensor']

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VaillantX6Component),
}).extend(cv.polling_component_schema('10s')).extend(uart.UART_DEVICE_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
