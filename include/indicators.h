// Status LED and buzzer. Deliberately coarse: an operator at a checkpoint
// should be able to read the device state without looking at the screen.
#pragma once
#include <stdint.h>

namespace bt {

enum class Signal : uint8_t { Off, Ready, Working, Breathe, Detected, Clear, Fault };

class Indicators {
public:
    void begin();
    void set(Signal s);
    void service();          // drives pulse/blink animation
    void beep(uint16_t hz, uint16_t ms);

private:
    Signal signal_ = Signal::Off;
    uint32_t phase_ms_ = 0;
    uint32_t beep_until_ms_ = 0;
    bool beeping_ = false;
};

}  // namespace bt
