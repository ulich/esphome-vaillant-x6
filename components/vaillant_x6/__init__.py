import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, binary_sensor, text_sensor, uart
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT, DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER

vaillant_x6_ns = cg.esphome_ns.namespace('vaillant_x6')
VaillantX6Component = vaillant_x6_ns.class_('VaillantX6Component', cg.PollingComponent)

DEPENDENCIES = ['uart']
AUTO_LOAD = ['uart', 'sensor', 'binary_sensor']

def multiple_of_10(value):
    value = cv.int_(value)
    if value < 10 or value % 10 != 0:
        raise cv.Invalid("poll_interval must be a multiple of 10")
    return value

def with_poll_interval(default_interval, schema):
    return schema.extend({
        cv.Optional('poll_interval', default=default_interval): multiple_of_10,
    })

def temperature_sensor_schema(icon=ICON_THERMOMETER):
    return sensor.sensor_schema(
        icon=icon,
        unit_of_measurement=UNIT_CELSIUS,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(VaillantX6Component),

        cv.Optional('circulating_pump_sensor'): with_poll_interval(10, binary_sensor.binary_sensor_schema(icon="mdi:pump")),
        cv.Optional('burner_sensor'): with_poll_interval(10, binary_sensor.binary_sensor_schema(icon="mdi:fire")),
        cv.Optional('flow_temperature_sensor'): with_poll_interval(10, temperature_sensor_schema()),
        cv.Optional('return_flow_temperature_sensor'): with_poll_interval(10, temperature_sensor_schema()),
        cv.Optional('flow_target_temperature_sensor'): with_poll_interval(60, temperature_sensor_schema(icon="mdi:thermometer-alert")),
        cv.Optional('room_thermostat_flow_target_temperature_sensor'): with_poll_interval(60, temperature_sensor_schema(icon="mdi:thermometer-alert")),
        cv.Optional('outside_temperature_sensor'): with_poll_interval(60, temperature_sensor_schema(icon="mdi:home-thermometer")),
    }
).extend(cv.polling_component_schema('10s')) \
.extend(uart.UART_DEVICE_SCHEMA)

async def add_temperature_sensor(name, request_bytes, config, var):
    if name in config:
        sensr = await sensor.new_sensor(config[name])
        cg.add(var.add_sensor(
            sensr,
            'AnalogueValue2Bytes',
            request_bytes,
            config[name]['poll_interval']))

async def add_binary_sensor(name, response_type, request_bytes, config, var):
    if name in config:
        sensr = await binary_sensor.new_binary_sensor(config[name])
        cg.add(var.add_binary_sensor(
            sensr,
            response_type,
            request_bytes,
            config[name]['poll_interval']))

def checksum(bytes):
    checksum = 0
    for current_byte in bytes:
        if checksum & 0x80:
            checksum = ((checksum << 1) | 1) & 0xFF
            checksum ^= 0x18
        else:
            checksum = (checksum << 1) & 0xFF
        checksum ^= current_byte
    return checksum

def request_bytes(command, expected_response_payload_length):
    bytes = [0x07, 0x00, 0x00, 0x00]
    bytes.append(command)
    bytes.append(expected_response_payload_length)
    cs = checksum(bytes)
    bytes.append(cs)
    return bytes


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    await add_binary_sensor('circulating_pump_sensor', 'Status01', request_bytes(0x44, 1), config, var)
    await add_binary_sensor('burner_sensor', 'Status0f', request_bytes(0x05, 1), config, var)

    await add_temperature_sensor('flow_temperature_sensor', request_bytes(0x18, 3), config, var)
    await add_temperature_sensor('return_flow_temperature_sensor', request_bytes(0x98, 5), config, var)
    await add_temperature_sensor('flow_target_temperature_sensor', request_bytes(0x39, 2), config, var)
    await add_temperature_sensor('room_thermostat_flow_target_temperature_sensor', request_bytes(0x25, 2), config, var)
    await add_temperature_sensor('outside_temperature_sensor', request_bytes(0x6a, 3), config, var)

    await uart.register_uart_device(var, config)
