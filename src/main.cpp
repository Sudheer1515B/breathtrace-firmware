// BreathTrace — breath-aerosol narcotics screening device.
// SIH 2026 · Problem Statement 26230
//
// SCREENING DEVICE, NOT AN EVIDENTIAL INSTRUMENT. A Detected result is a
// trigger for confirmatory laboratory testing and nothing more. See the
// limitations section of README.md before quoting any number this produces.
#include <Arduino.h>
#include "config.h"
#include "co2.h"
#include "flow.h"
#include "fluidics.h"
#include "indicators.h"
#include "power.h"
#include "reader.h"
#include "session.h"
#include "ui.h"

using namespace bt;

static Ui ui;
static StripReader reader;
static Co2Sensor co2;
static FlowSensor flow;
static Fluidics fluidics;
static Indicators indicators;
static Power power;
static Session session(ui, reader, co2, flow, fluidics, indicators, power);

// TEST button on an input-only pin, so the pull-up is external (10k to 3V3).
static constexpr uint32_t DEBOUNCE_MS = 40;
static bool btn_stable = true;          // true = released (pulled high)
static bool btn_last = true;
static uint32_t btn_changed_ms = 0;

static void service_button() {
    const bool raw = digitalRead(pin::BTN_TEST) != LOW;
    if (raw != btn_last) {
        btn_last = raw;
        btn_changed_ms = millis();
        return;
    }
    if (millis() - btn_changed_ms < DEBOUNCE_MS) return;
    if (raw == btn_stable) return;

    btn_stable = raw;
    if (!btn_stable) session.press_test();      // act on press, not release
}

void setup() {
    Serial.begin(115200);
    delay(150);
    Serial.println("\nBreathTrace firmware — PS 26230 — screening use only");

    pinMode(pin::BTN_TEST, INPUT);      // external pull-up: GPIO39 has none

    ui.begin();
    indicators.begin();
    power.begin();
    fluidics.begin();
    reader.begin();
    flow.begin();
    co2.begin();

    // No RTC in the bill of materials, so there is no trustworthy wall clock.
    // Timestamping a result for chain-of-custody needs one (e.g. DS3231);
    // until then the header shows that the time is unknown rather than lying.
    ui.set_clock("--:--");

    session.begin();
}

void loop() {
    service_button();
    session.update();
    delay(5);                           // flow detection needs >=50Hz polling
}
