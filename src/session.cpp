#include "session.h"
#include <Arduino.h>
#include <stdio.h>

namespace bt {

static constexpr uint8_t PUMP_ELUTE_DUTY = 190;

// Two consecutive kinetic reads whose control-line strength moves less than
// this are taken as the development endpoint.
static constexpr float ENDPOINT_DELTA = 0.010f;
static constexpr int   ENDPOINT_STABLE_READS = 2;

void Session::begin() {
    enter_(State::Idle);
}

uint32_t Session::elapsed_() const { return millis() - state_entered_ms_; }

void Session::enter_(State s) {
    state_ = s;
    state_entered_ms_ = millis();
}

void Session::abort_(State s, const char* reason) {
    reason_ = reason;
    fluidics_.pump_stop();
    fluidics_.heater(false);
    reader_.led(false);
    ind_.set(s == State::Fault ? Signal::Fault : Signal::Detected);
    ind_.beep(420, 600);
    enter_(s);
}

void Session::press_test() {
    switch (state_) {
        case State::Idle:
            if (power_.critical()) {
                abort_(State::Fault, "BATTERY TOO LOW");
                return;
            }
            if (!co2_.healthy()) {
                abort_(State::Fault, "CO2 SENSOR OFFLINE");
                return;
            }
            ind_.beep(1400, 90);
            enter_(State::Preheat);
            break;

        case State::Result:
        case State::Invalid:
        case State::Fault:
            enter_(State::Idle);
            break;

        default:
            break;      // mid-test presses are ignored, not an abort path
    }
}

void Session::update() {
    power_.poll();
    co2_.poll();
    fluidics_.service();
    ind_.service();

    switch (state_) {
        case State::Idle:     do_idle_();     break;
        case State::Preheat:  do_preheat_();  break;
        case State::Collect:  do_collect_();  break;
        case State::Elute:    do_elute_();    break;
        case State::Incubate: do_incubate_(); break;
        case State::Read:     do_read_();     break;
        case State::Result:   do_result_();   break;
        case State::Invalid:  ui_.screen_invalid(reason_); break;
        case State::Fault:    ui_.screen_fault(reason_);   break;
    }
}

void Session::do_idle_() {
    ind_.set(Signal::Ready);
    ui_.screen_idle(power_.percent());
}

// Warm the sample line before any breath enters it. Exhaled breath is
// saturated at body temperature, so a cold line condenses it out and a wet wall
// traps the drug-bearing aerosol before it ever reaches the filter.
void Session::do_preheat_() {
    ind_.set(Signal::Working);
    fluidics_.heater(true);

    if (elapsed_() < 400) {
        // Capture the blank-membrane white reference while the strip is still
        // dry. This doubles as the cassette-present check.
        if (!reader_.capture_baseline()) {
            abort_(State::Invalid, "NO CASSETTE FITTED");
            return;
        }
        flow_.auto_zero();
        flow_.reset_count();
        co2_.reset_window();
    }

    const int pct = static_cast<int>(elapsed_() * 100 / timing::PREHEAT_MS);
    ui_.screen_progress("WARMING LINE", "DO NOT BREATHE YET", pct);

    if (elapsed_() >= timing::PREHEAT_MS) {
        ind_.beep(1800, 120);
        enter_(State::Collect);
    }
}

// A single blow yields far too little aerosol. The published SensAbues protocol
// collects ~25 breaths over 2-3 minutes, which is what BREATHS_REQUIRED encodes.
void Session::do_collect_() {
    ind_.set(Signal::Breathe);
    if (flow_.poll()) ind_.beep(2200, 40);       // tick per counted breath

    ui_.screen_collecting(flow_.breath_count(), breath::BREATHS_REQUIRED,
                          co2_.ppm(), elapsed_() / 1000);

    if (flow_.breath_count() >= breath::BREATHS_REQUIRED) {
        // Alveolar-air gate. Window mean plus a peak requirement: the mean
        // proves sustained deep-lung air, the peak rejects a window of shallow
        // puffs that averages into range by accident.
        if (co2_.window_mean_ppm() < breath::CO2_MEAN_MIN_PPM ||
            co2_.window_peak_ppm() < breath::CO2_PEAK_MIN_PPM) {
            abort_(State::Invalid, "NOT DEEP-LUNG AIR");
            return;
        }
        co2_mean_at_result_ = co2_.window_mean_ppm();
        breaths_at_result_ = flow_.breath_count();
        enter_(State::Elute);
        return;
    }

    if (elapsed_() >= timing::COLLECT_MAX_MS) {
        abort_(State::Invalid, "TOO FEW BREATHS");
    }
}

void Session::do_elute_() {
    ind_.set(Signal::Working);
    fluidics_.heater(false);            // line is done; stop heating the strip
    fluidics_.pump(PUMP_ELUTE_DUTY);

    const int pct = static_cast<int>(elapsed_() * 100 / timing::ELUTE_MS);
    ui_.screen_progress("ELUTING SAMPLE", "WASHING FILTER", pct);

    if (elapsed_() >= timing::ELUTE_MS) {
        fluidics_.pump_stop();
        last_control_strength_ = -1.0f;
        stable_reads_ = 0;
        last_kinetic_ms_ = 0;
        enter_(State::Incubate);
    }
}

// Rather than wait a fixed development time, watch the control line and stop
// when it stops changing. A strip that develops fast returns a result sooner,
// and a slow one is not read before it is ready.
void Session::do_incubate_() {
    ind_.set(Signal::Working);

    const uint32_t el = elapsed_();
    const int pct = static_cast<int>(el * 100 / timing::INCUBATE_MAX_MS);
    ui_.screen_progress("DEVELOPING STRIP", "READING CONTROL LINE", pct);

    if (millis() - last_kinetic_ms_ >= sampling::KINETIC_INTERVAL_MS) {
        last_kinetic_ms_ = millis();
        const float c = reader_.mean_control_strength();
        if (last_control_strength_ >= 0.0f &&
            fabsf(c - last_control_strength_) < ENDPOINT_DELTA) {
            ++stable_reads_;
        } else {
            stable_reads_ = 0;
        }
        last_control_strength_ = c;
    }

    const bool settled = stable_reads_ >= ENDPOINT_STABLE_READS &&
                         el >= timing::INCUBATE_MIN_MS;
    if (settled || el >= timing::INCUBATE_MAX_MS) {
        enter_(State::Read);
    }
}

void Session::do_read_() {
    ui_.screen_progress("READING STRIP", "HOLD STILL", 95);

    LineRead raw[PANEL_COUNT];
    reader_.read_panels(raw);
    reader_.led(false);

    int invalid = 0, detected = 0;
    for (int p = 0; p < PANEL_COUNT; ++p) {
        const PanelCal cal{calib::RATIO_CUTOFF[p], calib::SIGMA_INTENSITY[p],
                           calib::CONTROL_MIN_INTENSITY, calib::CONFIDENCE_MIN};
        results_[p] = classify(raw[p], cal);
        if (results_[p].verdict == Verdict::Invalid) ++invalid;
        if (results_[p].verdict == Verdict::Detected) ++detected;

        Serial.printf("%s T=%.4f C=%.4f ratio=%.4f sig=%.5f conf=%.4f %s\n",
                      PANEL_NAME[p], raw[p].test, raw[p].control, results_[p].ratio,
                      results_[p].sigma_ratio, results_[p].confidence,
                      verdict_text(results_[p].verdict));
    }

    // Every control line missing means the cassette never developed at all,
    // which is a failed test rather than four separate failed panels.
    if (invalid == PANEL_COUNT) {
        abort_(State::Invalid, "NO CONTROL LINES");
        return;
    }

    ind_.set(detected > 0 ? Signal::Detected : Signal::Clear);
    ind_.beep(detected > 0 ? 700 : 1900, 250);
    enter_(State::Result);
}

void Session::do_result_() {
    ui_.screen_result(results_, co2_mean_at_result_, breaths_at_result_);
    if (elapsed_() >= timing::RESULT_HOLD_MS) enter_(State::Idle);
}

}  // namespace bt
