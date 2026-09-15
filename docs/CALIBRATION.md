# Calibration

`include/config.h` ships **placeholder** constants. They are plausible, not
measured. Flashing as-is gives a device that produces confident-looking
verdicts from arbitrary thresholds — worse than no device. Each procedure below
produces one number in the `calib` namespace.

---

## 1. `SIGMA_INTENSITY` — optical read noise

This is the number that makes the confidence figure mean anything, so measure
it first.

1. Fit a **developed, static** cassette. Do not run the pump.
2. Hold the device still, ambient light unchanged.
3. Log 30 consecutive reads of all eight channels (the serial monitor prints
   `T=` and `C=` per panel each time `do_read_` runs; or call
   `reader.read_panels()` in a loop from a scratch sketch).
4. For each panel, take the **standard deviation** of the test-line strengths
   and of the control-line strengths across the 30 reads.
5. Use the larger of the two per panel as that panel's `SIGMA_INTENSITY`.

Sanity check: a well-shielded reader should land around 0.005–0.02. Above ~0.05
means light is leaking into the cassette bay or the ADC is picking up pump
noise — fix the hardware rather than accepting the number, because a large
sigma widens the inconclusive band until nothing is ever decided.

## 2. `RATIO_CUTOFF` — per-panel decision threshold

Needs certified calibrator solutions at the cassette's stated cutoff
concentration. One cutoff per panel; they differ because the antibodies differ.

1. Prepare calibrators at 0×, 0.5×, 1×, and 2× the manufacturer's stated
   cutoff for that analyte.
2. Run at least 3 cassettes per level through the full elution and incubation
   path — not by pipetting onto the strip, since recovery through the filter and
   pump is part of what is being calibrated.
3. Plot measured T/C ratio against concentration.
4. Set `RATIO_CUTOFF[panel]` to the ratio observed at the 1× calibrator.

Record the dose–response curve. It is the evidence behind every verdict the
device gives, and it is the first thing a reviewer should ask to see.

## 3. `CONTROL_MIN_INTENSITY` — strip-developed threshold

1. Read several cassettes that **failed** to develop (run one dry, and one with
   the conjugate pad deliberately removed).
2. Read several known-good developed cassettes.
3. Set the threshold in the gap between the two populations, biased towards the
   failed side so a marginal strip reads `INVALID` rather than producing a
   verdict from a half-developed line.

## 4. Pressure sensor — `KPA_PER_COUNT` and `EXHALE_THRESHOLD_KPA`

`KPA_PER_COUNT` in `src/flow.cpp` is the ADC-count-to-pressure scale.

1. Tee a water manometer or a reference gauge into the mouthpiece line.
2. Apply a known pressure; record raw ADC counts against it at 3–4 points.
3. `KPA_PER_COUNT` = slope of that line.

Absolute accuracy barely matters — the breath detector works against an
auto-zeroed baseline, so only the **span** needs to be roughly right.

For `EXHALE_THRESHOLD_KPA`: have several people breathe normally through the
device and log `flow.pressure_kpa()`. Set the threshold well above the noise
floor but below the weakest real exhalation. Confirm the counted breaths match
a hand count over a 25-breath session; if it over-counts, raise
`EXHALE_MIN_MS` rather than the threshold, because coughs are short and strong.

## 5. CO₂ gate — `CO2_MEAN_MIN_PPM` and `CO2_PEAK_MIN_PPM`

The shipped values (12000 / 30000 ppm) assume normal end-tidal CO₂ of
4.6–5.9%, derated for dilution in the sample line and for the sensor's slow
response.

1. Run 5–10 cooperative subjects through a full valid collection; log
   `co2_.window_mean_ppm()` and `window_peak_ppm()`.
2. Run deliberate **bad** samples: shallow puffs, breathing past the
   mouthpiece, blowing room air through it.
3. Set thresholds to separate the two sets.

End-tidal CO₂ varies between individuals and with respiratory rate, so do not
tighten these to the point where a nervous subject fails a valid test. The gate
exists to reject room air and shallow puffs, not to grade breathing technique.

## 6. Battery — `VBAT_DIVIDER`, `VBAT_EMPTY`, `VBAT_FULL`

1. Measure the actual divider resistors and set `VBAT_DIVIDER` to
   `(R1 + R2) / R2`.
2. Compare `power.volts()` against a multimeter at the pack terminals; adjust
   `ADC_VREF` to close any residual offset.
3. Leave `VBAT_EMPTY` at 3.3 V or above — the pump browns out before the cell
   is actually flat.

---

## Verifying the maths, not the hardware

The classifier is Arduino-free and tested on the host:

```sh
pio test -e native
```

Those tests assert the polarity of a competitive assay, the 1.96σ confidence
floor, noise propagation from both lines, and that degenerate reads fail closed
to `INVALID` rather than into a verdict. They say nothing about whether the
calibration above was done — that is on you.
