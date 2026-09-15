#include "indicators.h"
#include "config.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

namespace bt {

#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
static constexpr int LEDC_BUZZER = 2;   // core 2.x needs an explicit channel; 3.x binds by pin
#endif
static Adafruit_NeoPixel strip(1, pin::STATUS_LED, NEO_GRB + NEO_KHZ800);

void Indicators::begin() {
    strip.begin();
    strip.setBrightness(90);
    strip.clear();
    strip.show();

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(pin::BUZZER, 2000, 10);
    ledcWrite(pin::BUZZER, 0);
#else
    ledcSetup(LEDC_BUZZER, 2000, 10);
    ledcAttachPin(pin::BUZZER, LEDC_BUZZER);
    ledcWrite(LEDC_BUZZER, 0);
#endif
}

void Indicators::set(Signal s) {
    signal_ = s;
    phase_ms_ = millis();
}

void Indicators::beep(uint16_t hz, uint16_t ms) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteTone(pin::BUZZER, hz);
#else
    ledcWriteTone(LEDC_BUZZER, hz);
#endif
    beeping_ = true;
    beep_until_ms_ = millis() + ms;
}

void Indicators::service() {
    const uint32_t now = millis();

    if (beeping_ && now >= beep_until_ms_) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWriteTone(pin::BUZZER, 0);
        ledcWrite(pin::BUZZER, 0);
#else
        ledcWriteTone(LEDC_BUZZER, 0);
        ledcWrite(LEDC_BUZZER, 0);
#endif
        beeping_ = false;
    }

    const uint32_t t = now - phase_ms_;
    uint8_t r = 0, g = 0, b = 0;
    switch (signal_) {
        case Signal::Off:      break;
        case Signal::Ready:    g = 40; b = 30; break;                 // steady teal
        case Signal::Working: {                                        // slow breathe
            const float k = 0.5f + 0.5f * sinf(t / 500.0f);
            r = static_cast<uint8_t>(234 * 0.35f * k);
            g = static_cast<uint8_t>(155 * 0.35f * k);
            b = static_cast<uint8_t>(86 * 0.35f * k);
            break;
        }
        case Signal::Breathe:  { const bool on = (t / 400) % 2;        // fast pulse
                                 if (on) { r = 234; g = 155; b = 86; } break; }
        case Signal::Detected: { const bool on = (t / 500) % 2;        // brick blink
                                 if (on) { r = 177; g = 87; b = 81; } break; }
        case Signal::Clear:    g = 120; b = 70; break;
        case Signal::Fault:    { const bool on = (t / 200) % 2;
                                 if (on) { r = 200; g = 20; b = 0; } break; }
    }
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

}  // namespace bt
