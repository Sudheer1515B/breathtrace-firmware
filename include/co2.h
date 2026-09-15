// MH-Z19B NDIR CO2 sensor over UART2 — alveolar-breath validity.
#pragma once
#include <stdint.h>

namespace bt {

class Co2Sensor {
public:
    void begin();

    // Poll at ~1Hz. Returns true when a fresh, checksum-valid reading arrived.
    bool poll();

    int  ppm() const { return ppm_; }
    bool healthy() const { return consecutive_errors_ < 5 && ever_read_; }

    // Window statistics, accumulated between reset_window() calls.
    void reset_window();
    int  window_mean_ppm() const;
    int  window_peak_ppm() const { return peak_; }
    uint32_t window_samples() const { return samples_; }

private:
    bool request_(uint8_t cmd);
    static uint8_t checksum_(const uint8_t* frame);   // bytes 1..7 of a 9-byte frame

    int ppm_ = 0;
    int peak_ = 0;
    uint64_t sum_ = 0;
    uint32_t samples_ = 0;
    uint32_t last_poll_ms_ = 0;
    int consecutive_errors_ = 0;
    bool ever_read_ = false;
};

}  // namespace bt
