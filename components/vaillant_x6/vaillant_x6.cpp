#include "vaillant_x6.h"
#include "esphome/core/application.h"
#include "response_decoder.h"

namespace esphome {
namespace vaillant_x6 {

void VaillantX6Component::setup() {
    request_response_handler = new RequestResponseHandler(
        std::bind(&uart::UARTDevice::read, this),
        std::bind(&uart::UARTDevice::write, this, std::placeholders::_1),
        std::bind(&uart::UARTDevice::available, this),
        std::bind(&VaillantX6Component::is_response_complete_, this),
        std::bind(&VaillantX6Component::is_response_valid, this)
    );

    {
        auto cmd = new GetOnOffStatusCommand(4);
        cmd->name = "Get Circulating Pump State";
        cmd->sensor_name = "Vaillant X6 Circulating Pump";
        cmd->object_id = "vaillant_x6_circulating_pump";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x44, 0x01, 0x69};
        cmd->icon = "mdi:pump";
        add_command(cmd);
    }
    {
        auto cmd = new GetOnOffStatusCommand(4);
        cmd->name = "Get Burner State";
        cmd->sensor_name = "Vaillant X6 Burner";
        cmd->object_id = "vaillant_x6_burner";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x05, 0x01, 0xEB};
        cmd->icon = "mdi:fire";
        cmd->on_value = 0x0f;
        add_command(cmd);
    }
    {
        auto cmd = new GetTemperatureCommand(6);
        cmd->name = "Get Flow Temperature";
        cmd->sensor_name = "Vaillant X6 Flow Temperature";
        cmd->object_id = "vaillant_x6_flow_temperature";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x18, 0x03, 0xd3};
        add_command(cmd);
    }
    {
        auto cmd = new GetTemperatureCommand(8);
        cmd->name = "Get Return Flow Temperature";
        cmd->sensor_name = "Vaillant X6 Return Flow Temperature";
        cmd->object_id = "vaillant_x6_return_flow_temperature";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x98, 0x05, 0xcc};
        add_command(cmd);
    }
    {
        auto cmd = new GetTemperatureCommand(5);
        cmd->name = "Get Flow Target Temperature";
        cmd->sensor_name = "Vaillant X6 Flow Target Temperature";
        cmd->object_id = "vaillant_x6_flow_target_temperature";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x39, 0x02, 0x90};
        cmd->icon = "mdi:thermometer-alert";
        cmd->interval = 6; // 10s polling => 60s
        add_command(cmd);
    }
    {
        auto cmd = new GetTemperatureCommand(5);
        cmd->name = "Get Room Thermostat Flow Target Temperature";
        cmd->sensor_name = "Vaillant X6 Room Thermostat Flow Target Temperature";
        cmd->object_id = "vaillant_x6_room_thermostat_flow_target_temperature";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x25, 0x02, 0xa8};
        cmd->icon = "mdi:thermometer-alert";
        cmd->interval = 6; // 10s polling => 60s
        add_command(cmd);
    }
        {
        auto cmd = new GetTemperatureCommand(6);
        cmd->name = "Get Outside Temperature";
        cmd->sensor_name = "Vaillant X6 Outside Temperature";
        cmd->object_id = "vaillant_x6_outside_temperature";
        cmd->request_bytes = {0x07, 0x00, 0x00, 0x00, 0x6a, 0x03, 0x37};
        cmd->icon = "mdi:home-thermometer";
        cmd->interval = 6; // 10s polling => 60s
        add_command(cmd);
    }
}

void VaillantX6Component::update() {
    current_command_idx = -1;
    seek_to_next_command();
    update_counter += 1;
}

void VaillantX6Component::loop() {
    auto response_available = request_response_handler->loop();

    if (response_available) {
        auto command = commands[current_command_idx];

        if (request_response_handler->bytes_read_count != command->get_required_response_length()) {
            ESP_LOGD(TAG, "Unexpected response length. Was %d, required %d",
                    request_response_handler->bytes_read_count, command->get_required_response_length());
            return;
        }

        command->process_response(request_response_handler->response_buffer);

        seek_to_next_command();
    }
}

void VaillantX6Component::seek_to_next_command() {
    current_command_idx++;
    while (current_command_idx < commands.size()) {
        auto& command = commands[current_command_idx];
        if ((update_counter % command->get_interval()) == 0) {
            request_response_handler->set_next_command(command);
            break;
        }
        current_command_idx++;
    }
}

bool VaillantX6Component::is_response_complete_() {
    uint8_t response_length = request_response_handler->response_buffer[0];

    return request_response_handler->bytes_read_count >= response_length;
}

bool VaillantX6Component::is_response_valid() {
    if (request_response_handler->response_buffer[1] != 0x00) {
        ESP_LOGD(TAG, "Second byte in response is not 0x00");
        return false;
    }

    uint8_t checksum = request_response_handler->response_buffer[request_response_handler->bytes_read_count - 1];
    if (calc_checksum_of_response() != checksum) {
        ESP_LOGD(TAG, "Checksum mismatch");
        return false;
    }

    return true;
}

uint8_t VaillantX6Component::calc_checksum_of_response() {
    uint8_t checksum = 0;

    for (int i = 0; i < request_response_handler->bytes_read_count - 1; i++) {
        uint8_t byte = request_response_handler->response_buffer[i];
    
        if (checksum & 0x80) {
            checksum = (checksum << 1) | 1;
            checksum = checksum ^ 0x18;
        } else {
            checksum = checksum << 1;
        }
        checksum = checksum ^ byte;
    }
    
    return checksum;
}

// -------------------------------------------- 

// The following methods should be in command.cpp but not sure how to make esphome aware of it
void GetTemperatureCommand::init_sensor() {
    sensor.set_object_id(object_id.c_str());
    sensor.set_name(sensor_name.c_str());
    sensor.set_icon(icon.c_str());
    sensor.set_state_class(sensor::STATE_CLASS_MEASUREMENT);
    sensor.set_device_class("temperature");
    sensor.set_unit_of_measurement("°C");
    sensor.set_accuracy_decimals(0);
    App.register_sensor(&sensor);
}

void GetTemperatureCommand::process_response(uint8_t* response) {
    float temperature = ResponseDecoder::temperature(response + 2);
    sensor.publish_state(temperature);
}

// -------------------------------------------- 

void GetOnOffStatusCommand::init_sensor() {
    sensor.set_object_id(object_id.c_str());
    sensor.set_name(sensor_name.c_str());
    sensor.set_icon(icon.c_str());
    App.register_binary_sensor(&sensor);
}

void GetOnOffStatusCommand::process_response(uint8_t* response) {
    bool is_on = response[2] == on_value;
    sensor.publish_state(is_on);
}

} // namespace vaillant_x6
} // namespace esphome
