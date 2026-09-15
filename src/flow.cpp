#include "flow.h"
#include "config.h"
#include <Arduino.h>

namespace bt {

// Transfer function for an analog 0.5-4.5V differential-pressure module scaled
// into the ESP32's 0-3.3V ADC range. The span is deliberately expressed as one
// constant so it can be corrected against a manometer without touching logic:
// see docs/CALIBRATION.md. Only relative pressure matters here — the breath
// threshold is defined against the auto-zeroed baseline, not absolute accuracy.
static constexpr float KPA_PER_COUNT = 0.00244f;

void FlowSensor::begin() {
    analogReadResolution(12);
    analogSetPinAttenuation(pin::PRESSURE, ADC_11db);
    auto_zero();
}

void FlowSensor::auto_zero() {
    uint32_t acc = 0;
    for (int i = 0; i < 128; ++i) {
        acc += analogRead(pin::PRESSURE);
        delayMicroseconds(200);
    }
    zero_counts_ = static_cast<float>(acc) / 128.0f;
}

float FlowSensor::read_kpa_() {
    uint32_t acc = 0;
    for (int i = 0; i < 8; ++i) acc += analogRead(pin::PRESSURE);
    const float counts = static_cast<float>(acc) / 8.0f;
    return (counts - zero_counts_) * KPA_PER_COUNT;
}

bool FlowSensor::poll() {
    kpa_ = read_kpa_();
    const uint32_t now = millis();

    if (!in_exhale_) {
        if (kpa_ > breath::EXHALE_THRESHOLD_KPA &&
            now - last_edge_ms_ > breath::EXHALE_REFRACTORY_MS) {
            in_exhale_ = true;
            exhale_start_ms_ = now;
        }
        return false;
    }

    // Still above threshold: keep waiting for the breath to finish.
    if (kpa_ > breath::EXHALE_THRESHOLD_KPA * 0.6f) return false;   // hysteresis

    in_exhale_ = false;
    last_edge_ms_ = now;
    if (now - exhale_start_ms_ >= breath::EXHALE_MIN_MS) {
        ++breaths_;
        return true;
    }
    return false;   // too brief to be a real exhalation — a cough or a bump
}

}  // namespace bt
