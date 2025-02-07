#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "request_response_handler.h"
#include "command.h"

namespace esphome {
namespace vaillant_x6 {

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
    
    void add_command(VaillantX6Command* cmd) {
      cmd->init_sensor();
      commands.push_back(cmd);
    }

    RequestResponseHandler *request_response_handler;
    std::vector<VaillantX6Command*> commands;
    int current_command_idx{-1};
    uint32_t update_counter{0};
};

} // namespace vaillant_x6
} // namespace esphome
