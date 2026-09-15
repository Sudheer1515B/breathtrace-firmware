#include "classifier.h"
#include <cmath>

namespace bt {

static constexpr float SQRT2 = 1.41421356237f;

float read_confidence(float ratio, float sigma_ratio, float cutoff) {
    if (!(sigma_ratio > 0.0f)) return 0.0f;          // also catches NaN
    const float z = std::fabs(ratio - cutoff) / sigma_ratio;
    return std::erf(z / SQRT2);                       // == 2*Phi(z) - 1
}

float ratio_sigma(const LineRead& r, float sigma_intensity) {
    if (!(r.test > 0.0f) || !(r.control > 0.0f)) return 0.0f;
    const float ratio = r.test / r.control;
    const float rel_t = sigma_intensity / r.test;
    const float rel_c = sigma_intensity / r.control;
    return ratio * std::sqrt(rel_t * rel_t + rel_c * rel_c);
}

PanelResult classify(const LineRead& r, const PanelCal& cal) {
    PanelResult out{Verdict::Invalid, 0.0f, 0.0f, 0.0f};

    // A missing control line means the assay never ran. That is a hardware or
    // consumable failure, not an ambiguous result, and the operator's next
    // action differs: fit a new strip rather than escalate to a lab.
    if (!(r.control >= cal.control_min)) return out;
    if (!(r.test >= 0.0f)) return out;

    out.ratio = r.test / r.control;
    out.sigma_ratio = ratio_sigma(r, cal.sigma_intensity);
    out.confidence = read_confidence(out.ratio, out.sigma_ratio, cal.ratio_cutoff);

    if (out.confidence < cal.confidence_min) {
        out.verdict = Verdict::Inconclusive;
    } else {
        // Competitive format: free drug in the sample blocks the conjugate, so
        // a FAINT test line means drug present. Low ratio => Detected.
        out.verdict = (out.ratio < cal.ratio_cutoff) ? Verdict::Detected
                                                     : Verdict::Clear;
    }
    return out;
}

const char* verdict_text(Verdict v) {
    switch (v) {
        case Verdict::Clear:        return "CLEAR";
        case Verdict::Detected:     return "DETECTED";
        case Verdict::Inconclusive: return "INCONCL";
        case Verdict::Invalid:      return "INVALID";
    }
    return "INVALID";
}

}  // namespace bt
