// BreathTrace — hardware configuration and tunables.
// Every pin and threshold the firmware depends on lives here. See
// docs/PINOUT.md for the wiring table and docs/CALIBRATION.md for how the
// numbers in the CALIBRATION block are obtained (they are NOT guesses to be
// trusted as-is — they must be measured per build).
#pragma once
#include <stdint.h>

// ---------------------------------------------------------------- pin map
// ESP32-WROOM-32. GPIO 34-39 are input-only and have no internal pull-ups.
namespace pin {
// Display (ILI9488 SPI TFT). RST tied to EN, backlight tied to 3V3.
constexpr int TFT_SCK   = 18;
constexpr int TFT_MOSI  = 23;
constexpr int TFT_CS    = 13;
constexpr int TFT_DC    = 14;

// MH-Z19B NDIR CO2 sensor, UART2.
constexpr int CO2_RX    = 16;   // ESP32 receives on this pin <- sensor TX
constexpr int CO2_TX    = 17;

// CD74HC4051 8-channel analog mux: the eight strip photodiodes.
constexpr int MUX_S0    = 25;
constexpr int MUX_S1    = 26;
constexpr int MUX_S2    = 27;
constexpr int MUX_OUT   = 34;   // ADC1_CH6, input-only

// Strip illumination: white LED bar over the cassette window, PWM dimmed.
constexpr int READER_LED = 33;

// Fluidics.
constexpr int PUMP      = 32;   // peristaltic pump via MOSFET, PWM
constexpr int HEATER    = 4;    // PTC element via MOSFET, on/off (self-regulating)

// Operator interface.
constexpr int BUZZER    = 21;
constexpr int STATUS_LED = 22;  // single WS2812
constexpr int BTN_TEST  = 39;   // input-only: needs an external 10k pull-up

// Analog monitoring.
constexpr int PRESSURE  = 35;   // ADC1_CH7, differential pressure -> breath flow
constexpr int VBAT      = 36;   // ADC1_CH0, 18650 through a 2:1 divider
}  // namespace pin

// ---------------------------------------------------------------- panels
// A 4-panel competitive LFIA cassette. Order must match the physical channel
// order of the photodiode array, left to right across the cassette.
constexpr int PANEL_COUNT = 4;
constexpr int CHANNEL_COUNT = PANEL_COUNT * 2;   // one test + one control each

// Mux channel assignment. Test and control for a panel are adjacent.
constexpr uint8_t MUX_TEST[PANEL_COUNT]    = {0, 2, 4, 6};
constexpr uint8_t MUX_CONTROL[PANEL_COUNT] = {1, 3, 5, 7};

extern const char* const PANEL_NAME[PANEL_COUNT];   // "THC", "OPI", "MET", "AMP"

// ---------------------------------------------------------------- timing
namespace timing {
constexpr uint32_t PREHEAT_MS       = 25000;   // warm the sample line before collecting
constexpr uint32_t COLLECT_MAX_MS   = 240000;  // hard stop on the collection window
constexpr uint32_t ELUTE_MS         = 12000;   // pump run to wash the filter onto the strip
constexpr uint32_t INCUBATE_MIN_MS  = 120000;  // never call an endpoint before this
constexpr uint32_t INCUBATE_MAX_MS  = 420000;  // ...and never wait longer than this
constexpr uint32_t RESULT_HOLD_MS   = 120000;  // auto-return to idle after this
constexpr uint32_t READER_SETTLE_US = 2500;    // mux settle + photodiode rise before sampling
}  // namespace timing

// ---------------------------------------------------------------- sampling
namespace sampling {
constexpr int ADC_AVERAGES   = 64;    // per channel per read, to beat ESP32 ADC noise
constexpr int ADC_MAX        = 4095;  // 12-bit
constexpr uint8_t LED_DUTY   = 180;   // 0-255 illumination level, fixed during a session
constexpr uint32_t KINETIC_INTERVAL_MS = 10000;  // strip re-read cadence while incubating
}  // namespace sampling

// ---------------------------------------------------------------- breath gate
namespace breath {
// Alveolar-air validity. Normal end-tidal CO2 is 35-45 mmHg, about 4.6-5.9%
// of one atmosphere. The MH-Z19B is far too slow (T90 tens of seconds) to
// resolve a per-breath capnogram, so it is used as a WINDOW-MEAN check that
// the subject really is exhaling deep-lung air, not a waveform gate.
// Per-breath plateau detection would need a fast sensor (e.g. SprintIR-W).
constexpr int CO2_MEAN_MIN_PPM = 12000;   // ~1.2% averaged over the whole window
constexpr int CO2_PEAK_MIN_PPM = 30000;   // ~3.0% must be reached at least once
constexpr int CO2_SENSOR_MAX_PPM = 50000; // MH-Z19B 0-50000ppm variant; clips above

// Breath counting comes from the differential-pressure sensor, which is fast
// enough to see individual exhalations.
constexpr int BREATHS_REQUIRED = 25;      // the published SensAbues collection protocol
constexpr float EXHALE_THRESHOLD_KPA = 0.15f;  // above auto-zeroed baseline
constexpr uint32_t EXHALE_MIN_MS = 600;   // ignore shorter pressure blips
constexpr uint32_t EXHALE_REFRACTORY_MS = 400;
}  // namespace breath

// ---------------------------------------------------------------- calibration
// MEASURE THESE. Shipping the defaults unchanged produces meaningless verdicts.
// docs/CALIBRATION.md gives the bench procedure for each.
namespace calib {
// Normalised T/C ratio below which a competitive strip reads as drug-present.
// One per panel: the panels have different antibody affinities and cutoffs.
constexpr float RATIO_CUTOFF[PANEL_COUNT] = {0.55f, 0.55f, 0.55f, 0.55f};

// 1-sigma optical read noise, in normalised intensity units, from repeatedly
// reading one static strip. Propagated into the ratio, not assumed on it.
constexpr float SIGMA_INTENSITY[PANEL_COUNT] = {0.012f, 0.012f, 0.012f, 0.012f};

// A control line dimmer than this means the strip never developed: INVALID,
// which is a different outcome from INCONCLUSIVE and needs a new strip.
constexpr float CONTROL_MIN_INTENSITY = 0.18f;

// Read confidence below this is reported as INCONCLUSIVE rather than a verdict.
constexpr float CONFIDENCE_MIN = 0.95f;

// Battery divider ratio and ADC reference, for the fuel gauge.
constexpr float VBAT_DIVIDER = 2.0f;
constexpr float ADC_VREF = 3.30f;
constexpr float VBAT_EMPTY = 3.30f;
constexpr float VBAT_FULL  = 4.15f;
}  // namespace calib
