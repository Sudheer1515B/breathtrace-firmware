#include "fluidics.h"
#include "config.h"
#include <Arduino.h>

namespace bt {

#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
static constexpr int LEDC_PUMP = 1;   // core 2.x needs an explicit channel; 3.x binds by pin
#endif
static constexpr uint32_t RAMP_STEP_MS = 20;
static constexpr uint8_t RAMP_STEP = 8;

void Fluidics::begin() {
    pinMode(pin::HEATER, OUTPUT);
    digitalWrite(pin::HEATER, LOW);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(pin::PUMP, 1000, 8);
    ledcWrite(pin::PUMP, 0);
#else
    ledcSetup(LEDC_PUMP, 1000, 8);
    ledcAttachPin(pin::PUMP, LEDC_PUMP);
    ledcWrite(LEDC_PUMP, 0);
#endif
}

void Fluidics::heater(bool on) {
    heater_on_ = on;
    digitalWrite(pin::HEATER, on ? HIGH : LOW);
}

void Fluidics::pump(uint8_t duty) {
    pump_target_ = duty;
    if (duty == 0) {                 // stopping is immediate, starting is ramped
        pump_now_ = 0;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(pin::PUMP, 0);
#else
        ledcWrite(LEDC_PUMP, 0);
#endif
    }
}

void Fluidics::service() {
    if (pump_now_ == pump_target_) return;
    const uint32_t now = millis();
    if (now - last_ramp_ms_ < RAMP_STEP_MS) return;
    last_ramp_ms_ = now;

    if (pump_now_ < pump_target_) {
        const int next = pump_now_ + RAMP_STEP;
        pump_now_ = (next > pump_target_) ? pump_target_ : static_cast<uint8_t>(next);
    } else {
        const int next = pump_now_ - RAMP_STEP;
        pump_now_ = (next < pump_target_) ? pump_target_ : static_cast<uint8_t>(next);
    }
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin::PUMP, pump_now_);
#else
    ledcWrite(LEDC_PUMP, pump_now_);
#endif
}

}  // namespace bt
