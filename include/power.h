// 18650 fuel gauge off a 2:1 divider. Voltage-only, so it is a coarse estimate:
// no coulomb counting, and it sags under pump load.
#pragma once
#include <stdint.h>

namespace bt {

class Power {
public:
    void begin();
    void poll();
    float volts() const { return volts_; }
    int   percent() const;
    bool  critical() const { return volts_ > 0.5f && volts_ < 3.40f; }

private:
    float volts_ = 0.0f;
};

}  // namespace bt
