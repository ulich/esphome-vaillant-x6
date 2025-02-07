#include "vaillant_x6.h"
#include "response_decoder.h"
#include "esphome/core/application.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome {
namespace vaillant_x6 {

class GetCirculatingPumpStatusCommand : public VaillantX6Command {
  public:
    GetCirculatingPumpStatusCommand() : VaillantX6Command(4) {
        name = "Get Circulating Pump State";
        request_bytes = {0x07, 0x00, 0x00, 0x00, 0x44, 0x01, 0x69};

        sensor.set_object_id("vaillant_x6_circulating_pump");
        sensor.set_name("Vaillant X6 Circulating Pump");
        sensor.set_icon("mdi:pump");
        App.register_binary_sensor(&sensor);
    }

    virtual void process_response(uint8_t* response) override {
        bool isOn = response[2] == 0x01;
        sensor.publish_state(isOn);
    }

  private:
    binary_sensor::BinarySensor sensor;
};

class GetBurnerStatusCommand : public VaillantX6Command {
  public:
    GetBurnerStatusCommand() : VaillantX6Command(4) {
        name = "Get Burner State";
        request_bytes = {0x07, 0x00, 0x00, 0x00, 0x05, 0x01, 0xEB};

        sensor.set_object_id("vaillant_x6_burner");
        sensor.set_name("Vaillant X6 Burner");
        sensor.set_icon("mdi:fire");
        App.register_binary_sensor(&sensor);
    }

    virtual void process_response(uint8_t* response) override {
        bool isOn = response[2] == 0x0f;
        sensor.publish_state(isOn);
    }

  private:
    binary_sensor::BinarySensor sensor;
};

class GetFlowTargetTemperatureCommand : public VaillantX6Command {
  public:
    GetFlowTargetTemperatureCommand() : VaillantX6Command(5) {
        name = "Get Flow Target Temperature";
        request_bytes = {0x07, 0x00, 0x00, 0x00, 0x39, 0x02, 0x90};

        sensor.set_object_id("vaillant_x6_flow_target_temperature");
        sensor.set_name("Vaillant X6 Flow Target Temperature");
        sensor.set_icon("mdi:thermometer-alert");
        sensor.set_accuracy_decimals(0);
        sensor.set_state_class(sensor::STATE_CLASS_MEASUREMENT);
        sensor.set_device_class("temperature");
        sensor.set_unit_of_measurement("°C");
        App.register_sensor(&sensor);
    }

    virtual void process_response(uint8_t* response) override {
        float temperature = ResponseDecoder::temperature(response + 2);
        sensor.publish_state(temperature);
    }

    virtual int get_interval() override {
        return 6; // every 60s since polling component is set to 10s
    }

  private:
    sensor::Sensor sensor;
};

class GetFlowTemperatureCommand : public VaillantX6Command {
  public:
    GetFlowTemperatureCommand() : VaillantX6Command(6) {
        name = "Get Flow Temperature";
        request_bytes = {0x07, 0x00, 0x00, 0x00, 0x18, 0x03, 0xd3};

        sensor.set_object_id("vaillant_x6_flow_temperature");
        sensor.set_name("Vaillant X6 Flow Temperature");
        sensor.set_icon("mdi:thermometer");
        sensor.set_accuracy_decimals(0);
        sensor.set_state_class(sensor::STATE_CLASS_MEASUREMENT);
        sensor.set_device_class("temperature");
        sensor.set_unit_of_measurement("°C");
        App.register_sensor(&sensor);
    }

    virtual void process_response(uint8_t* response) override {
        float temperature = ResponseDecoder::temperature(response + 2);
        sensor.publish_state(temperature);
    }

  private:
    sensor::Sensor sensor;
};

class GetReturnFlowTemperatureCommand : public VaillantX6Command {
  public:
    GetReturnFlowTemperatureCommand() : VaillantX6Command(8) {
        name = "Get Return Flow Temperature";
        request_bytes = {0x07, 0x00, 0x00, 0x00, 0x98, 0x05, 0xcc};

        sensor.set_object_id("vaillant_x6_return_flow_temperature");
        sensor.set_name("Vaillant X6 Return Flow Temperature");
        sensor.set_icon("mdi:thermometer");
        sensor.set_accuracy_decimals(0);
        sensor.set_state_class(sensor::STATE_CLASS_MEASUREMENT);
        sensor.set_device_class("temperature");
        sensor.set_unit_of_measurement("°C");
        App.register_sensor(&sensor);
    }

    virtual void process_response(uint8_t* response) override {
        float temperature = ResponseDecoder::temperature(response + 2);
        sensor.publish_state(temperature);
    }

  private:
    sensor::Sensor sensor;
};


void VaillantX6Component::setup() {
    request_response_handler = new RequestResponseHandler(
        std::bind(&uart::UARTDevice::read, this),
        std::bind(&uart::UARTDevice::write, this, std::placeholders::_1),
        std::bind(&uart::UARTDevice::available, this),
        std::bind(&VaillantX6Component::is_response_complete_, this),
        std::bind(&VaillantX6Component::is_response_valid, this)
    );

    commands.push_back(new GetCirculatingPumpStatusCommand());
    commands.push_back(new GetBurnerStatusCommand());
    commands.push_back(new GetFlowTargetTemperatureCommand());
    commands.push_back(new GetFlowTemperatureCommand());
    commands.push_back(new GetReturnFlowTemperatureCommand());
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

} // namespace vaillant_x6
} // namespace esphome
