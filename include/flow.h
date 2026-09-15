// Differential-pressure breath detector — counts exhalations.
//
// The CO2 sensor is far too slow to resolve individual breaths; this is what
// actually drives the breath count that gates the collection window.
#pragma once
#include <stdint.h>

namespace bt {

class FlowSensor {
public:
    void begin();

    // Re-learn the no-flow baseline. Call with the mouthpiece clear.
    void auto_zero();

    // Call often (>=50Hz). Returns true on the falling edge of a breath that
    // was long enough to count, i.e. once per completed exhalation.
    bool poll();

    int  breath_count() const { return breaths_; }
    void reset_count() { breaths_ = 0; }
    float pressure_kpa() const { return kpa_; }
    bool  exhaling() const { return in_exhale_; }

private:
    float read_kpa_();

    float zero_counts_ = 0.0f;
    float kpa_ = 0.0f;
    int breaths_ = 0;
    bool in_exhale_ = false;
    uint32_t exhale_start_ms_ = 0;
    uint32_t last_edge_ms_ = 0;
};

}  // namespace bt
