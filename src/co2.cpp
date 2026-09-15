#include "co2.h"
#include "config.h"
#include <Arduino.h>

namespace bt {

static HardwareSerial& kPort = Serial2;

// MH-Z19B command frames. Byte 0 is always 0xFF (start), byte 1 the sensor id,
// byte 2 the command, byte 8 the checksum.
static const uint8_t CMD_READ[9] =
    {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
// Auto Baseline Correction OFF. ABC assumes the sensor periodically sees 400ppm
// fresh air and re-zeros to it. A breath sampler violates that assumption
// badly, so leaving ABC on lets the zero drift with use. Must stay off.
static const uint8_t CMD_ABC_OFF[9] =
    {0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86};

uint8_t Co2Sensor::checksum_(const uint8_t* frame) {
    uint8_t sum = 0;
    for (int i = 1; i < 8; ++i) sum += frame[i];
    return static_cast<uint8_t>(0xFF - sum + 1);
}

void Co2Sensor::begin() {
    kPort.begin(9600, SERIAL_8N1, pin::CO2_RX, pin::CO2_TX);
    delay(50);
    kPort.write(CMD_ABC_OFF, sizeof(CMD_ABC_OFF));
    kPort.flush();
    reset_window();
}

bool Co2Sensor::request_(uint8_t cmd) {
    (void)cmd;
    while (kPort.available()) kPort.read();          // drop stale bytes
    kPort.write(CMD_READ, sizeof(CMD_READ));
    kPort.flush();

    uint8_t rx[9];
    const uint32_t deadline = millis() + 120;
    int got = 0;
    while (got < 9 && millis() < deadline) {
        if (kPort.available()) {
            const uint8_t b = static_cast<uint8_t>(kPort.read());
            if (got == 0 && b != 0xFF) continue;      // resynchronise on start byte
            rx[got++] = b;
        }
    }
    if (got < 9 || rx[1] != 0x86 || rx[8] != checksum_(rx)) {
        ++consecutive_errors_;
        return false;
    }

    const int value = (static_cast<int>(rx[2]) << 8) | rx[3];
    if (value < 0 || value > breath::CO2_SENSOR_MAX_PPM) {
        ++consecutive_errors_;
        return false;
    }

    ppm_ = value;
    consecutive_errors_ = 0;
    ever_read_ = true;
    sum_ += static_cast<uint64_t>(value);
    ++samples_;
    if (value > peak_) peak_ = value;
    return true;
}

bool Co2Sensor::poll() {
    const uint32_t now = millis();
    if (now - last_poll_ms_ < 1000) return false;     // sensor updates at ~1Hz
    last_poll_ms_ = now;
    return request_(0x86);
}

void Co2Sensor::reset_window() {
    sum_ = 0;
    samples_ = 0;
    peak_ = 0;
}

int Co2Sensor::window_mean_ppm() const {
    if (samples_ == 0) return 0;
    return static_cast<int>(sum_ / samples_);
}

}  // namespace bt
