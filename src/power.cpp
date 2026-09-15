#include "power.h"
#include "config.h"
#include <Arduino.h>

namespace bt {

void Power::begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(pin::VBAT, ADC_11db);
    poll();
}

void Power::poll() {
    uint32_t acc = 0;
    for (int i = 0; i < 32; ++i) acc += analogRead(pin::VBAT);
    const float counts = static_cast<float>(acc) / 32.0f;
    const float measured = counts / static_cast<float>(sampling::ADC_MAX) * calib::ADC_VREF;
    const float v = measured * calib::VBAT_DIVIDER;
    // Light smoothing: the pump makes this jump around.
    volts_ = (volts_ < 0.5f) ? v : (volts_ * 0.8f + v * 0.2f);
}

int Power::percent() const {
    const float span = calib::VBAT_FULL - calib::VBAT_EMPTY;
    if (span <= 0.0f) return 0;
    float pct = (volts_ - calib::VBAT_EMPTY) / span * 100.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return static_cast<int>(pct + 0.5f);
}

}  // namespace bt
