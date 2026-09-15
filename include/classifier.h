// Strip-reading maths: normalised line intensities in, verdict + confidence out.
//
// This translation unit is deliberately free of Arduino/ESP32 dependencies so
// it can be unit-tested on the host (`pio test -e native`). It contains the
// only arithmetic in the firmware that decides anything, so it is the part
// worth testing.
#pragma once
#include <stdint.h>

namespace bt {

enum class Verdict : uint8_t {
    Clear,          // test line strong  -> no drug above cutoff
    Detected,       // test line faint   -> drug present above cutoff
    Inconclusive,   // ran fine, but too close to the cutoff to call
    Invalid,        // control line absent: the strip did not develop
};

// One panel's optical read. Both values are normalised reflectance in [0,1],
// already ambient-subtracted and referenced to the blank membrane.
struct LineRead {
    float test;
    float control;
};

// Per-panel calibration. See docs/CALIBRATION.md.
struct PanelCal {
    float ratio_cutoff;       // T/C below this reads as drug-present
    float sigma_intensity;    // 1-sigma optical read noise, intensity units
    float control_min;        // control dimmer than this => Invalid
    float confidence_min;     // below this => Inconclusive
};

struct PanelResult {
    Verdict verdict;
    float ratio;        // measured T/C
    float sigma_ratio;  // propagated 1-sigma on the ratio
    float confidence;   // read confidence in [0,1]; 0 when Invalid
};

// Read confidence: the probability that the measured ratio is genuinely on the
// side of the cutoff it appears to be on, given optical read noise alone.
//
//   z    = |ratio - cutoff| / sigma_ratio
//   conf = 2*Phi(z) - 1 = erf(z / sqrt(2))
//
// This is NOT a clinical probability of drug use. It excludes cross-reactivity,
// aerosol recovery variability, and dose/time uncertainty. Calibrating those
// out would take a validated breath dataset that does not currently exist for
// anything but THC.
float read_confidence(float ratio, float sigma_ratio, float cutoff);

// Propagate per-line intensity noise into the ratio:
//   sigma_ratio = ratio * sqrt((sigma/T)^2 + (sigma/C)^2)
float ratio_sigma(const LineRead& r, float sigma_intensity);

PanelResult classify(const LineRead& r, const PanelCal& cal);

const char* verdict_text(Verdict v);   // "CLEAR", "DETECTED", "INCONCL", "INVALID"

}  // namespace bt
