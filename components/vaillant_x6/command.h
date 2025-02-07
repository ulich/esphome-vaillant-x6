#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "request_response_handler.h"

namespace esphome {
namespace vaillant_x6 {

class VaillantX6Command : public Command {
  public:
    VaillantX6Command(int required_response_length)
    : required_response_length(required_response_length) {}

    virtual ~VaillantX6Command() = default;
    virtual void init_sensor() = 0;
    virtual void process_response(uint8_t* response) = 0;
    
    virtual int get_required_response_length() {
        return required_response_length;
    }

    virtual int get_interval() {
      return 1;
    }
  
  private:
    int required_response_length = 0;
};

class GetTemperatureCommand : public VaillantX6Command {
  public:
    GetTemperatureCommand(int required_response_length)
    : VaillantX6Command(required_response_length) {}

    virtual void init_sensor() override;
    virtual void process_response(uint8_t* response) override;

    virtual int get_interval() override {
        return interval;
    }

    std::string object_id;
    std::string icon = "mdi:thermometer";
    std::string sensor_name;
    int interval = 1;

  private:
    sensor::Sensor sensor;
};

class GetOnOffStatusCommand : public VaillantX6Command {
  public:
    GetOnOffStatusCommand(int required_response_length)
    : VaillantX6Command(required_response_length) {}

    virtual void init_sensor() override;
    virtual void process_response(uint8_t* response) override;

    virtual int get_interval() override {
        return interval;
    }

    std::string object_id;
    std::string icon;
    std::string sensor_name;
    int interval = 1;
    uint8_t on_value = 0x01;

  private:
    binary_sensor::BinarySensor sensor;
};

} // namespace vaillant_x6
} // namespace esphome
