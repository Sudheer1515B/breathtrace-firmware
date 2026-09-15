#include "reader.h"
#include <Arduino.h>

namespace bt {

// LEDC channel reserved for the illumination bar.
#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
static constexpr int LEDC_READER = 0;   // core 2.x needs an explicit channel; 3.x binds by pin
#endif

void StripReader::select_(uint8_t mux_ch) {
    digitalWrite(pin::MUX_S0, (mux_ch & 0x01) ? HIGH : LOW);
    digitalWrite(pin::MUX_S1, (mux_ch & 0x02) ? HIGH : LOW);
    digitalWrite(pin::MUX_S2, (mux_ch & 0x04) ? HIGH : LOW);
    delayMicroseconds(timing::READER_SETTLE_US);
}

void StripReader::begin() {
    pinMode(pin::MUX_S0, OUTPUT);
    pinMode(pin::MUX_S1, OUTPUT);
    pinMode(pin::MUX_S2, OUTPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(pin::MUX_OUT, ADC_11db);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(pin::READER_LED, 5000, 8);
#else
    ledcSetup(LEDC_READER, 5000, 8);
    ledcAttachPin(pin::READER_LED, LEDC_READER);
#endif
    led(false);
}

void StripReader::led(bool on) {
    const uint8_t duty = on ? sampling::LED_DUTY : 0;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(pin::READER_LED, duty);
#else
    ledcWrite(LEDC_READER, duty);
#endif
}

// Ambient-subtracted reflectance for one channel, normalised to full scale.
// The dark read is taken immediately after the lit read on the same channel so
// the two share the same ambient conditions.
float StripReader::sample_channel_(uint8_t mux_ch) {
    select_(mux_ch);

    led(true);
    delayMicroseconds(timing::READER_SETTLE_US);
    uint32_t lit = 0;
    for (int i = 0; i < sampling::ADC_AVERAGES; ++i) lit += analogRead(pin::MUX_OUT);

    led(false);
    delayMicroseconds(timing::READER_SETTLE_US);
    uint32_t dark = 0;
    for (int i = 0; i < sampling::ADC_AVERAGES; ++i) dark += analogRead(pin::MUX_OUT);

    const float signal = (static_cast<float>(lit) - static_cast<float>(dark)) /
                         static_cast<float>(sampling::ADC_AVERAGES);
    const float norm = signal / static_cast<float>(sampling::ADC_MAX);
    return norm > 0.0f ? norm : 0.0f;
}

bool StripReader::capture_baseline() {
    bool ok = true;
    for (uint8_t ch = 0; ch < CHANNEL_COUNT; ++ch) {
        baseline_[ch] = sample_channel_(ch);
        // A blank membrane should reflect strongly. Anything dim here means no
        // cassette, a misseated cassette, or a failed LED — not a faint line.
        if (baseline_[ch] < 0.10f) ok = false;
    }
    has_baseline_ = ok;
    return ok;
}

// Line strength: how much darker this position is than the blank membrane was.
// 0.0 = no line, approaching 1.0 = a fully saturated line.
static inline float strength(float now, float baseline) {
    if (!(baseline > 0.0f)) return 0.0f;
    const float s = (baseline - now) / baseline;
    if (s < 0.0f) return 0.0f;
    if (s > 1.0f) return 1.0f;
    return s;
}

void StripReader::read_panels(LineRead out[PANEL_COUNT]) {
    for (int p = 0; p < PANEL_COUNT; ++p) {
        const uint8_t tch = MUX_TEST[p];
        const uint8_t cch = MUX_CONTROL[p];
        out[p].test    = strength(sample_channel_(tch), baseline_[tch]);
        out[p].control = strength(sample_channel_(cch), baseline_[cch]);
    }
}

float StripReader::mean_control_strength() {
    float acc = 0.0f;
    for (int p = 0; p < PANEL_COUNT; ++p) {
        const uint8_t cch = MUX_CONTROL[p];
        acc += strength(sample_channel_(cch), baseline_[cch]);
    }
    return acc / static_cast<float>(PANEL_COUNT);
}

}  // namespace bt
