#pragma once

class ResponseDecoder {
  public:
    static float analogueValue2Bytes(uint8_t* response) {
        int16_t i = (response[0] << 8) | response[1];
        return i / (16.0f);
    }
};
