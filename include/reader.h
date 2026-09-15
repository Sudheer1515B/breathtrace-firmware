// Optical strip reader: 8 photodiodes through a CD74HC4051 analog mux, under a
// white LED bar illuminating the whole cassette window.
//
// Why eight channels and not one sensor: a 4-panel cassette has four test lines
// and four control lines, so there are eight positions to measure. A single
// point sensor can only see one of them without a moving head.
//
// Why each channel is its own reference: the strip is blank at every line
// position before the sample arrives, so a baseline captured at that moment is
// a per-channel white reference. Using it cancels LED non-uniformity,
// photodiode gain spread and ambient offset in one step, which is what makes
// cheap discrete optics usable here.
#pragma once
#include <stdint.h>
#include "classifier.h"
#include "config.h"

namespace bt {

struct ChannelRaw {
    float lit;      // normalised reflectance with the LED on, ambient-subtracted
    float dark;     // ambient only, LED off
};

class StripReader {
public:
    void begin();

    // Capture the blank-membrane white reference. Call after the cassette is
    // seated and BEFORE elution. Returns false if any channel is too dark to
    // reference against, which means no cassette or a dead LED.
    bool capture_baseline();
    bool has_baseline() const { return has_baseline_; }

    // Read all four panels as line strengths relative to the baseline.
    void read_panels(LineRead out[PANEL_COUNT]);

    // Mean control-line strength, used for incubation endpoint detection.
    float mean_control_strength();

    void led(bool on);

private:
    float sample_channel_(uint8_t mux_ch);
    static void select_(uint8_t mux_ch);

    float baseline_[CHANNEL_COUNT] = {0};
    bool has_baseline_ = false;
};

}  // namespace bt
