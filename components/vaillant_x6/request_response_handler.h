#pragma once

#define VAILLANT_X6_SERIAL_BUFFER_LEN 100
#define VAILLANT_X6_RESPONSE_TIMEOUT 2000

namespace esphome {
namespace vaillant_x6 {

static const char *TAG = "vaillant_x6";

class Command {
  public:
    virtual ~Command() = default;

    std::string name;
    std::vector<uint8_t> request_bytes;
};

class RequestResponseHandler {
  public:
    RequestResponseHandler(
        std::function<uint8_t()> read,
        std::function<void(uint8_t)> write,
        std::function<bool()> available,
        std::function<bool()> is_response_complete,
        std::function<bool()> is_response_valid
    ) : read_(read),
        write(write),
        available(available),
        is_response_complete(is_response_complete),
        is_response_valid(is_response_valid) {
    }

    void set_next_command(Command* command) {
        current_command_ = command;
        is_waiting_for_response = false;
    }

    bool loop() {
        if (is_waiting_for_response) {
            if (millis() - last_request_time_ > VAILLANT_X6_RESPONSE_TIMEOUT) {
                ESP_LOGD(TAG, "Response timeout");
                is_waiting_for_response = false;
                return false;
            }

            while (available()) {
                if (bytes_read_count > VAILLANT_X6_SERIAL_BUFFER_LEN) {
                    ESP_LOGD(TAG, "Read buffer full");
                    is_waiting_for_response = false;
                    return false;
                }

                response_buffer[bytes_read_count] = read_();
                bytes_read_count++;
            }

            if (bytes_read_count > 0) {
                if (!is_response_complete()) {
                    return false;
                }

                if (!is_response_valid()) {
                    ESP_LOGD(TAG, "Invalid response received");
                    is_waiting_for_response = false;
                    return false;
                }

                ESP_LOGD(TAG, "Valid response received");
                is_waiting_for_response = false;
                current_command_ = nullptr;
                return true;
            }
        } else if (current_command_ != nullptr) {
            ESP_LOGD(TAG, "Sending request: %s", current_command_->name.c_str());
            send_request_();
        }
        return false;
    }

    int bytes_read_count{0};
    uint8_t response_buffer[VAILLANT_X6_SERIAL_BUFFER_LEN] = {0};

  private:
    void send_request_() {
        for (uint8_t byte : current_command_->request_bytes) {
            write(byte);
        }

        last_request_time_ = millis();
        is_waiting_for_response = true;
        bytes_read_count = 0;
    }

    std::function<uint8_t()> read_;
    std::function<void(uint8_t)> write;
    std::function<bool()> available;
    std::function<bool()> is_response_complete;
    std::function<bool()> is_response_valid;

    Command* current_command_{nullptr};
    bool is_waiting_for_response{false};
    uint32_t last_request_time_{0};
};

} // namespace vaillant_x6
} // namespace esphome
