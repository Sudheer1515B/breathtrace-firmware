#pragma once
#include <stdint.h>
class TFT_eSPI {
public:
    void init();
    void setRotation(uint8_t);
    uint16_t color565(uint8_t, uint8_t, uint8_t);
    void fillScreen(uint16_t);
    void setTextFont(uint8_t);
    void setTextSize(uint8_t);
    int32_t height();
    void fillRect(int32_t, int32_t, int32_t, int32_t, uint16_t);
    void setTextColor(uint16_t, uint16_t);
    void drawChar(uint16_t, int32_t, int32_t, uint8_t);
    int32_t width();
};
