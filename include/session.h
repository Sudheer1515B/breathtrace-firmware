// Test-session state machine.
//
// One pass runs: Idle -> Preheat -> Collect -> Elute -> Incubate -> Read ->
// Result. Two exits leave that path early and mean different things to the
// operator, which is why they are separate states:
//   Invalid — the test did not run (no cassette, no control line, no alveolar
//             air). Fit a new cassette and repeat.
//   Fault   — the device is not fit to test (sensor dead, battery critical).
#pragma once
#include <stdint.h>
#include "classifier.h"
#include "config.h"
#include "co2.h"
#include "flow.h"
#include "fluidics.h"
#include "indicators.h"
#include "power.h"
#include "reader.h"
#include "ui.h"

namespace bt {

enum class State : uint8_t {
    Idle, Preheat, Collect, Elute, Incubate, Read, Result, Invalid, Fault
};

class Session {
public:
    Session(Ui& ui, StripReader& reader, Co2Sensor& co2, FlowSensor& flow,
            Fluidics& fluidics, Indicators& ind, Power& power)
        : ui_(ui), reader_(reader), co2_(co2), flow_(flow),
          fluidics_(fluidics), ind_(ind), power_(power) {}

    void begin();
    void update();
    void press_test();                  // TEST button, already debounced

    State state() const { return state_; }
    const PanelResult* results() const { return results_; }

private:
    void enter_(State s);
    void abort_(State s, const char* reason);
    uint32_t elapsed_() const;

    void do_idle_();
    void do_preheat_();
    void do_collect_();
    void do_elute_();
    void do_incubate_();
    void do_read_();
    void do_result_();

    Ui& ui_;
    StripReader& reader_;
    Co2Sensor& co2_;
    FlowSensor& flow_;
    Fluidics& fluidics_;
    Indicators& ind_;
    Power& power_;

    State state_ = State::Idle;
    uint32_t state_entered_ms_ = 0;
    const char* reason_ = "";

    PanelResult results_[PANEL_COUNT] = {};
    int co2_mean_at_result_ = 0;
    int breaths_at_result_ = 0;

    // Incubation endpoint tracking.
    uint32_t last_kinetic_ms_ = 0;
    float last_control_strength_ = -1.0f;
    int stable_reads_ = 0;
};

}  // namespace bt
