#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "request_response_handler.h"

namespace esphome {
namespace vaillant_x6 {

class VaillantX6Command : public Command {
  public:
    VaillantX6Command(int required_response_length)
    : required_response_length(required_response_length) {}

    virtual ~VaillantX6Command() = default;
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

class VaillantX6Component : public PollingComponent, public uart::UARTDevice {
  public:
    void setup() override;
    
    // called every 16ms
    void loop() override;

    // called on every update_interval configured in __init__.py
    void update() override;

  protected:
    bool is_response_complete_();
    bool is_response_valid();
    uint8_t calc_checksum_of_response();
    void seek_to_next_command();

    RequestResponseHandler *request_response_handler;
    std::vector<VaillantX6Command*> commands;
    int current_command_idx{-1};
    uint32_t update_counter{0};
};

} // namespace vaillant_x6
} // namespace esphome
