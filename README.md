# BreathTrace firmware

ESP32 firmware for a handheld breath-aerosol narcotics screening device.
**SIH 2026 · Problem Statement 26230.**

> **Screening device, not an evidential instrument.** A `DETECTED` result is a
> trigger for confirmatory laboratory testing and nothing more. Read
> [Limitations](#limitations) before quoting any number this firmware produces.

## What it does

Exhaled breath carries non-volatile drugs (THC, opioids) not as vapour but as
micro-droplets of bronchiole lining fluid, ~0.3–2 µm across. A gas sensor
cannot see them at all. So the device captures the aerosol physically, elutes
it onto a commodity competitive lateral-flow immunoassay cassette, and reads
the strip optically.

```
mouthpiece ──▶ heated line ──▶ aerosol filter ──▶ elution ──▶ 4-panel LFIA ──▶ optical read
                    │                                                              │
              breath validity                                              T/C ratio ──▶ verdict
        (CO2 window + breath count)                                           + read confidence
```

Panels: **THC**, **OPI** (morphine/opiates), **MET** (methamphetamine),
**AMP** (amphetamine).

## Hardware

| Function | Part |
|---|---|
| MCU | ESP32-WROOM-32 |
| Display | **1.8" ST7735 SPI TFT, 128×160** (or ILI9488 480×320 — see Build) |
| Breath validity | MH-Z19B NDIR CO₂ sensor (UART) |
| Breath counting | analog differential-pressure sensor |
| Strip reader | 8 photodiodes → CD74HC4051 mux, white LED bar |
| Elution | 12 V peristaltic dosing pump via MOSFET |
| Line heating | self-regulating PTC element via MOSFET |
| Operator I/O | WS2812 status LED, piezo buzzer, TEST button |
| Power | 18650 + TP4056, 2:1 divider to ADC |

Full wiring table: **[docs/PINOUT.md](docs/PINOUT.md)**.

## Build

```sh
pio run                      # compile for the 1.8" ST7735 (default)
pio run -e ili9488           # ...or for the ~4" ILI9488
pio run -t upload            # flash
pio device monitor           # 115200 baud, prints per-panel raw reads
pio test -e native           # host-side classifier tests
```

**No PlatformIO?** `sh tools/check.sh` syntax-checks every source against stub
headers (both Arduino core 2.x and 3.x) and runs the classifier tests with plain
g++. It exits non-zero on failure. It is not a substitute for `pio run` — it
never touches a real toolchain, linker or the ESP32 headers — but it catches
syntax errors, bad signatures and broken classifier maths. `tools/stubs/unity.h`
is a shim so `test/test_classifier` has two runners rather than two copies.

TFT_eSPI is configured entirely through `build_flags` in `platformio.ini`, so
no library file needs hand-editing. Those pins must stay in step with
`include/config.h`.

**Both panels run one UI.** `src/ui.cpp` lays its 20×7 character grid out from
`tft.width()` / `tft.height()` at startup, picking the font and pitches to
suit — font 1 (6×8) on the ST7735, font 4 (26px) on the ILI9488. There is no
second UI implementation to keep in sync.

**If the ST7735 image is offset by a few pixels or the colours look inverted**,
change `ST7735_GREENTAB3` in `platformio.ini`. These modules ship with several
different panel tabs (`GREENTAB`, `GREENTAB2`, `REDTAB`, `BLACKTAB`) that need
different row/column offsets, and there is no way to detect which you have
except by trying them.

## How the decision is made

Competitive assay, so the polarity is inverted from intuition: **a strong test
line means no drug.** Free drug in the sample blocks the conjugate, so a faint
line means drug present.

```
ratio       = I_test / I_control
sigma_ratio = ratio * sqrt((sigma/I_test)^2 + (sigma/I_control)^2)
z           = |ratio - cutoff| / sigma_ratio
confidence  = erf(z / sqrt(2))          # == 2*Phi(z) - 1
```

| condition | verdict |
|---|---|
| control line below `CONTROL_MIN_INTENSITY` | `INVALID` |
| confidence < 95% | `INCONCLUSIVE` |
| confidence ≥ 95%, ratio < cutoff | `DETECTED` |
| confidence ≥ 95%, ratio ≥ cutoff | `CLEAR` |

The 95% floor lands on exactly ±1.96σ, which is what makes the inconclusive
band defensible rather than arbitrary. `test/test_classifier` asserts this.

`INVALID` and `INCONCLUSIVE` are deliberately different outcomes, because the
operator's next action differs: `INVALID` means fit a new cassette, whereas
`INCONCLUSIVE` means the test ran fine and should escalate to a lab.

Two design choices worth knowing:

- **Each channel is its own white reference.** The strip is blank at every line
  position before the sample arrives, so a baseline captured then cancels LED
  non-uniformity, photodiode gain spread and ambient offset in one step. That
  is what makes cheap discrete optics usable here.
- **Incubation ends on an endpoint, not a timer.** The control line is re-read
  every 10 s and development is called complete when it stops changing, so a
  fast strip returns sooner and a slow one is not read before it is ready.

## Limitations

These are real and should be stated out loud rather than discovered by a judge.

- **`confidence` is read confidence, not clinical confidence.** It covers
  optical and strip-line noise only. It excludes cross-reactivity, aerosol
  recovery variability, and dose/time uncertainty. Calibrating those out needs
  a validated breath dataset that does not exist today for anything but THC.
- **No published breath accuracy exists for opioids, amphetamines or cocaine.**
  Only THC has it. The OPI/MET/AMP panels are implemented because the cassette
  provides them, not because breath performance is characterised.
- **CO₂ gating is a window mean, not a capnogram.** The MH-Z19B's T90 is tens
  of seconds — far too slow to resolve a per-breath waveform. It confirms the
  subject sustained deep-lung air across the window. Per-breath plateau gating
  would need a fast sensor such as a SprintIR-W.
- **No RTC.** There is no trustworthy wall clock, so the header shows `--:--`
  rather than a fabricated time. Chain-of-custody timestamping needs a DS3231.
- **Aerosol recovery efficiency is unmeasured.** Mouthpiece and filter losses
  fold into one empirical figure that has to be measured on the bench.
- **Calibration constants ship as placeholders.** `include/config.h` carries
  plausible numbers, not measured ones. Running as-shipped produces
  meaningless verdicts — see [docs/CALIBRATION.md](docs/CALIBRATION.md).

## Layout

```
include/config.h      pins, timings, thresholds, calibration constants
include/classifier.h  ratio -> verdict maths (Arduino-free, host-testable)
src/classifier.cpp
src/reader.cpp        mux scan, ambient subtraction, per-channel white reference
src/co2.cpp           MH-Z19B UART protocol, ABC disabled
src/flow.cpp          differential-pressure exhalation counter
src/fluidics.cpp      pump ramp + PTC heater switching
src/ui.cpp            20x7 display grid, row-cached
src/indicators.cpp    WS2812 + buzzer
src/power.cpp         18650 fuel gauge
src/session.cpp       test state machine
src/main.cpp          setup/loop, debounced TEST button
test/test_classifier  host-side unit tests
docs/                 pinout, calibration, state machine
```

## Licence

MIT — see [LICENSE](LICENSE).
