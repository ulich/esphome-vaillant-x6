#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "request_response_handler.h"

namespace esphome {
namespace vaillant_x6 {

class VaillantX6Command : public Command {
  public:
    virtual ~VaillantX6Command() = default;
    virtual void process_response(uint8_t* response) = 0;
    virtual int get_expected_response_length();

    virtual int get_interval() {
      return 1;
    }
  
  private:
    int expected_response_length = 0;
};

class GetAnalogueValue2BytesCommand : public VaillantX6Command {
  public:
    virtual void process_response(uint8_t* response) override;

    virtual int get_interval() override {
        return interval;
    }

    int interval = 1;
    sensor::Sensor* sensor;
};

class GetOnOffStatusCommand : public VaillantX6Command {
  public:
    virtual void process_response(uint8_t* response) override;

    virtual int get_interval() override {
        return interval;
    }

    std::string object_id;
    std::string icon;
    std::string sensor_name;
    int interval = 1;
    uint8_t on_value = 0x01;
    binary_sensor::BinarySensor* sensor;
};

} // namespace vaillant_x6
} // namespace esphome
