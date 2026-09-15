# Pinout — ESP32-WROOM-32

Pins are common to both display builds except TFT RST, noted below.

Authoritative source is `include/config.h`. The TFT pins are duplicated in
`platformio.ini` build flags because TFT_eSPI is configured at compile time —
**change both together.**

| Signal | GPIO | Direction | Notes |
|---|---|---|---|
| TFT SCK | 18 | out | VSPI clock |
| TFT MOSI | 23 | out | |
| TFT CS | 13 | out | |
| TFT DC | 14 | out | |
| TFT RST | 19 | out | ST7735 build only; ILI9488 build ties it to EN |
| TFT backlight | — | — | tie to 3V3 |
| CO₂ RX | 16 | in | to MH-Z19B **TX** |
| CO₂ TX | 17 | out | to MH-Z19B **RX** |
| Mux S0 | 25 | out | CD74HC4051 |
| Mux S1 | 26 | out | |
| Mux S2 | 27 | out | |
| Mux common out | 34 | analog in | ADC1_CH6, input-only |
| Reader LED bar | 33 | out (PWM) | via MOSFET or transistor |
| Pump | 32 | out (PWM) | logic-level MOSFET gate |
| PTC heater | 4 | out | MOSFET gate, on/off only |
| Buzzer | 21 | out (PWM) | piezo |
| Status LED | 22 | out | single WS2812 |
| TEST button | 39 | in | **needs an external 10 kΩ pull-up** |
| Pressure sensor | 35 | analog in | ADC1_CH7, input-only |
| Battery sense | 36 | analog in | ADC1_CH0, 2:1 divider |

## Constraints this layout respects

- **All analog inputs are on ADC1** (GPIO 32–39). ADC2 is unusable while Wi-Fi
  is active, so anything analog must live on ADC1.
- **GPIO 34–39 are input-only and have no internal pull-ups.** That is why the
  TEST button needs an external resistor; `INPUT_PULLUP` silently does nothing
  on these pins.
- **No strapping pins are used as outputs** (0, 2, 5, 12, 15 are avoided), so
  boot mode and the boot log are unaffected.
- GPIO 19 carries the display reset on the ST7735 build. The ILI9488 build
  leaves it free (that panel is happy with RST tied to EN), so it is available
  there for a display read path or a second SPI peripheral.

## Mux channel map

A 4-panel cassette has eight optical positions. Test and control for each panel
are on adjacent mux channels, ordered left-to-right across the cassette:

| Panel | Test ch | Control ch |
|---|---|---|
| THC | 0 | 1 |
| OPI | 2 | 3 |
| MET | 4 | 5 |
| AMP | 6 | 7 |

If the photodiode array is wired in a different physical order, correct
`MUX_TEST` / `MUX_CONTROL` in `include/config.h` rather than rewiring.

## Power notes

- The pump is the dominant load and will sag the 18650 rail. `Fluidics::pump()`
  ramps duty rather than stepping it, because a cold start at full duty stalls
  a peristaltic head and can brown out the ESP32.
- The PTC element is self-regulating, so there is no thermistor and no control
  loop — it is switched, not servoed.
