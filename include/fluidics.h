// Peristaltic elution pump and the self-regulating PTC line heater.
#pragma once
#include <stdint.h>

namespace bt {

class Fluidics {
public:
    void begin();

    // The PTC element regulates its own temperature, so there is no control
    // loop here and no thermistor — it is switched, not servoed.
    void heater(bool on);
    bool heater_on() const { return heater_on_; }

    // 0-255. Ramped rather than stepped: a cold start at full duty stalls a
    // peristaltic head and can pull the rail down enough to brown out the ESP32.
    void pump(uint8_t duty);
    void pump_stop() { pump(0); }

    // Call from the main loop to advance the pump ramp.
    void service();

private:
    bool heater_on_ = false;
    uint8_t pump_target_ = 0;
    uint8_t pump_now_ = 0;
    uint32_t last_ramp_ms_ = 0;
};

}  // namespace bt
