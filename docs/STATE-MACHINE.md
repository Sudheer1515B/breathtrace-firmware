# Test session state machine

Implemented in `src/session.cpp`.

```
              ┌──────────────────────────────────────────────┐
              │                                              │
              ▼                                              │
        ┌──────────┐  TEST                                   │
        │   IDLE   │────────┐                                │
        └──────────┘        │                                │
           ▲   ▲            ▼                                │
           │   │      ┌──────────┐  no cassette              │
           │   │      │ PREHEAT  │──────────────┐            │
           │   │      └──────────┘              │            │
           │   │            │ 25 s              │            │
           │   │            ▼                   │            │
           │   │      ┌──────────┐  <25 breaths │            │
           │   │      │ COLLECT  │──────────────┤            │
           │   │      └──────────┘  or room air │            │
           │   │            │ 25 breaths        │            │
           │   │            │ + CO2 gate passed │            │
           │   │            ▼                   ▼            │
           │   │      ┌──────────┐         ┌─────────┐       │
           │   │      │  ELUTE   │         │ INVALID │───────┤ TEST
           │   │      └──────────┘         └─────────┘       │
           │   │            │ 12 s                          │
           │   │            ▼                               │
           │   │      ┌──────────┐                          │
           │   │      │ INCUBATE │                          │
           │   │      └──────────┘                          │
           │   │            │ control line settled          │
           │   │            ▼                               │
           │   │      ┌──────────┐  no control lines        │
           │   │      │   READ   │──────────────────────────┤
           │   │      └──────────┘                          │
           │   │            │                               │
           │   │            ▼                               │
           │   │      ┌──────────┐  TEST, or 120 s          │
           │   └──────│  RESULT  │──────────────────────────┘
           │          └──────────┘
           │          ┌──────────┐  TEST
           └──────────│  FAULT   │  (battery critical / CO2 sensor offline)
                      └──────────┘
```

| State | Does | Leaves when |
|---|---|---|
| `Idle` | shows ready + battery | TEST pressed |
| `Preheat` | heater on; captures strip white reference; auto-zeros flow | 25 s elapsed |
| `Collect` | counts exhalations, accumulates CO₂ window | 25 breaths **and** CO₂ gate passes |
| `Elute` | pump ramps up, washes filter onto strip | 12 s elapsed |
| `Incubate` | re-reads control line every 10 s | control line stops changing (min 2 min, max 7 min) |
| `Read` | scans 8 channels, classifies 4 panels | always → `Result` |
| `Result` | shows per-panel verdict + read confidence | TEST pressed, or 2 min |
| `Invalid` | test did not run — fit a new cassette | TEST pressed |
| `Fault` | device unfit to test | TEST pressed |

## Design notes

**Why preheat before any breath enters.** Exhaled breath leaves the lungs near
body temperature and essentially saturated. A cold line condenses it out, and a
wet wall is an efficient droplet trap — it would capture the drug-bearing
aerosol before the filter ever sees it.

**Why the white reference is captured during preheat.** The strip is still dry
and blank at every line position, so each channel's own reading is a valid
white reference. It doubles as the cassette-present check: a channel too dark
to reference against means no cassette, a misseated one, or a dead LED.

**Why 25 breaths and not one blow.** A single exhalation carries far too little
aerosol mass. 25 breaths over 2–3 minutes matches the published SensAbues
collection protocol.

**Why the CO₂ gate needs both a mean and a peak.** The mean proves the subject
sustained deep-lung air across the window. The peak rejects a window of shallow
puffs that happens to average into range.

**Why mid-test TEST presses are ignored.** Once the pump has eluted onto the
strip, the cassette is consumed. Letting a stray press abort would waste a
consumable and leave the fluidics in an unknown state; there is no way to
un-elute. Fault conditions still abort on their own.

**Why `Invalid` is not `Fault`.** `Invalid` means the device is fine and the
test needs repeating with a new cassette. `Fault` means the device itself needs
attention. Collapsing them would send an operator hunting for a hardware
problem that does not exist, or worse, let them retest on a device that is
broken.
