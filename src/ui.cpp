#include "ui.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <stdio.h>
#include <string.h>

namespace bt {

static TFT_eSPI tft;

static constexpr int COLS      = 20;
static constexpr int COL_PITCH = 21;
static constexpr int MARGIN_X  = 30;
static constexpr int FONT      = 4;
static constexpr int GLYPH_H   = 26;

// Nine paint slots: 7 text rows and 2 rules, indexed in screen order.
static constexpr int SLOT_COUNT = 9;
// Slots 2 and 7 are the horizontal rules; the rest carry text.
static constexpr int SLOT_Y[SLOT_COUNT] = {18, 52, 88, 96, 130, 164, 198, 236, 244};

void Ui::begin() {
    tft.init();
    tft.setRotation(1);                 // landscape, 480x320
    c_bg_     = tft.color565(5, 9, 10);
    c_orange_ = tft.color565(234, 155, 86);
    c_brick_  = tft.color565(177, 87, 81);
    c_teal_   = tft.color565(159, 201, 196);
    c_white_  = tft.color565(232, 242, 243);
    c_dim_    = tft.color565(110, 138, 140);
    c_rule_   = tft.color565(48, 66, 68);
    tft.fillScreen(c_bg_);
    tft.setTextFont(FONT);
    invalidate_();
}

void Ui::set_clock(const char* hhmm) {
    snprintf(clock_, sizeof(clock_), "%s", hhmm);
}

void Ui::invalidate_() {
    for (int i = 0; i < SLOT_COUNT; ++i) cache_valid_[i] = false;
}

void Ui::rule_(int index) {
    if (cache_valid_[index]) return;
    const int y = SLOT_Y[index];
    tft.fillRect(MARGIN_X, y, COLS * COL_PITCH, 3, c_rule_);
    cache_valid_[index] = true;
}

// Compose the row into a fixed 20-column buffer, compare against what is
// already on the glass, and repaint only if it differs.
void Ui::row_(int index, const Seg* segs, int n) {
    char line[COLS + 1];
    uint16_t cols[COLS];
    memset(line, ' ', COLS);
    line[COLS] = 0;
    for (int i = 0; i < COLS; ++i) cols[i] = c_white_;

    for (int s = 0; s < n; ++s) {
        const char* t = segs[s].text;
        for (int i = 0; t[i] && (segs[s].col + i) < COLS; ++i) {
            line[segs[s].col + i] = t[i];
            cols[segs[s].col + i] = segs[s].color;
        }
    }

    if (cache_valid_[index] &&
        memcmp(cache_[index], line, COLS) == 0 &&
        memcmp(cache_col_[index], cols, sizeof(cols)) == 0) {
        return;
    }

    const int y = SLOT_Y[index];
    tft.fillRect(0, y, tft.width(), GLYPH_H, c_bg_);
    for (int i = 0; i < COLS; ++i) {
        if (line[i] == ' ') continue;
        tft.setTextColor(cols[i], c_bg_);
        tft.drawChar(static_cast<uint16_t>(line[i]), MARGIN_X + i * COL_PITCH, y, FONT);
    }

    memcpy(cache_[index], line, COLS);
    memcpy(cache_col_[index], cols, sizeof(cols));
    cache_valid_[index] = true;
}

// ------------------------------------------------------------------ screens
void Ui::screen_idle(int battery_pct) {
    char batt[12];
    snprintf(batt, sizeof(batt), "BAT %3d%%", battery_pct);
    const Seg r0[] = {{0, "BREATHTRACE", c_orange_}, {15, clock_, c_dim_}};
    const Seg r1[] = {{0, "READY", c_teal_}, {12, batt, c_dim_}};
    row_(0, r0, 2);
    row_(1, r1, 2);
    rule_(2);
    const Seg blank[] = {{0, "", c_dim_}};
    const Seg msg[]   = {{0, "PRESS TEST TO BEGIN", c_white_}};
    const Seg hint[]  = {{0, "FIT A NEW CASSETTE", c_dim_}};
    row_(3, msg, 1);
    row_(4, hint, 1);
    row_(5, blank, 1);
    row_(6, blank, 1);
    rule_(7);
    const Seg foot[] = {{3, "SCREENING ONLY", c_dim_}};
    row_(8, foot, 1);
}

void Ui::screen_progress(const char* label, const char* detail, int pct) {
    char bar[21];
    const int filled = (pct < 0 ? 0 : pct > 100 ? 100 : pct) * COLS / 100;
    for (int i = 0; i < COLS; ++i) bar[i] = (i < filled) ? '=' : '.';
    bar[COLS] = 0;

    const Seg r0[] = {{0, "BREATHTRACE", c_orange_}, {15, clock_, c_dim_}};
    const Seg r1[] = {{0, label, c_teal_}};
    const Seg r3[] = {{0, detail, c_white_}};
    const Seg r5[] = {{0, bar, c_orange_}};
    const Seg blank[] = {{0, "", c_dim_}};
    row_(0, r0, 2);
    row_(1, r1, 1);
    rule_(2);
    row_(3, r3, 1);
    row_(4, blank, 1);
    row_(5, r5, 1);
    row_(6, blank, 1);
    rule_(7);
    const Seg foot[] = {{3, "SCREENING ONLY", c_dim_}};
    row_(8, foot, 1);
}

void Ui::screen_collecting(int breaths, int required, int co2_ppm, uint32_t elapsed_s) {
    char hdr[16], detail[24], co2[20], bar[21];
    snprintf(hdr, sizeof(hdr), "COLLECTING");
    snprintf(detail, sizeof(detail), "BREATHS %2d/%2d", breaths, required);
    snprintf(co2, sizeof(co2), "CO2 %2d.%01d%%  %02u:%02u",
             co2_ppm / 10000, (co2_ppm / 1000) % 10,
             static_cast<unsigned>(elapsed_s / 60), static_cast<unsigned>(elapsed_s % 60));

    const int filled = required > 0 ? (breaths * COLS / required) : 0;
    for (int i = 0; i < COLS; ++i) bar[i] = (i < filled) ? '=' : '.';
    bar[COLS] = 0;

    const Seg r0[] = {{0, "BREATHTRACE", c_orange_}, {15, clock_, c_dim_}};
    const Seg r1[] = {{0, hdr, c_teal_}};
    const Seg r3[] = {{0, "KEEP BREATHING OUT", c_white_}};
    const Seg r4[] = {{0, detail, c_orange_}};
    const Seg r5[] = {{0, bar, c_orange_}};
    const Seg r6[] = {{0, co2, c_dim_}};
    row_(0, r0, 2);
    row_(1, r1, 1);
    rule_(2);
    row_(3, r3, 1);
    row_(4, r4, 1);
    row_(5, r5, 1);
    row_(6, r6, 1);
    rule_(7);
    const Seg foot[] = {{3, "SCREENING ONLY", c_dim_}};
    row_(8, foot, 1);
}

void Ui::screen_result(const PanelResult results[PANEL_COUNT], int co2_mean_ppm,
                       int breaths) {
    char sample[24];
    snprintf(sample, sizeof(sample), "SAMPLE OK  CO2 %d.%01d%%",
             co2_mean_ppm / 10000, (co2_mean_ppm / 1000) % 10);
    (void)breaths;

    const Seg r0[] = {{0, "BREATHTRACE", c_orange_}, {15, clock_, c_dim_}};
    const Seg r1[] = {{0, sample, c_teal_}};
    row_(0, r0, 2);
    row_(1, r1, 1);
    rule_(2);

    for (int p = 0; p < PANEL_COUNT; ++p) {
        const PanelResult& r = results[p];
        char conf[6];
        if (r.verdict == Verdict::Invalid) {
            snprintf(conf, sizeof(conf), " --");
        } else {
            int pct = static_cast<int>(r.confidence * 100.0f + 0.5f);
            if (pct > 99) pct = 99;          // never print a bare 100%
            snprintf(conf, sizeof(conf), "%2d%%", pct);
        }
        uint16_t vc = c_teal_;
        if (r.verdict == Verdict::Detected)     vc = c_orange_;
        else if (r.verdict == Verdict::Invalid) vc = c_brick_;
        else if (r.verdict == Verdict::Inconclusive) vc = c_dim_;

        const Seg row[] = {{0, PANEL_NAME[p], c_white_},
                           {6, verdict_text(r.verdict), vc},
                           {17, conf, c_dim_}};
        row_(3 + p, row, 3);
    }

    rule_(7);
    const Seg foot[] = {{3, "SCREENING ONLY", c_dim_}};
    row_(8, foot, 1);
}

void Ui::screen_invalid(const char* reason) {
    const Seg r0[] = {{0, "BREATHTRACE", c_orange_}, {15, clock_, c_dim_}};
    const Seg r1[] = {{0, "TEST INVALID", c_brick_}};
    const Seg r3[] = {{0, reason, c_white_}};
    const Seg r4[] = {{0, "FIT A NEW CASSETTE", c_dim_}};
    const Seg r5[] = {{0, "AND RETEST", c_dim_}};
    const Seg blank[] = {{0, "", c_dim_}};
    row_(0, r0, 2);
    row_(1, r1, 1);
    rule_(2);
    row_(3, r3, 1);
    row_(4, r4, 1);
    row_(5, r5, 1);
    row_(6, blank, 1);
    rule_(7);
    const Seg foot[] = {{3, "NOT A RESULT", c_brick_}};
    row_(8, foot, 1);
}

void Ui::screen_fault(const char* reason) {
    const Seg r0[] = {{0, "BREATHTRACE", c_orange_}, {15, clock_, c_dim_}};
    const Seg r1[] = {{0, "FAULT", c_brick_}};
    const Seg r3[] = {{0, reason, c_white_}};
    const Seg blank[] = {{0, "", c_dim_}};
    row_(0, r0, 2);
    row_(1, r1, 1);
    rule_(2);
    row_(3, r3, 1);
    row_(4, blank, 1);
    row_(5, blank, 1);
    row_(6, blank, 1);
    rule_(7);
    const Seg foot[] = {{3, "NOT A RESULT", c_brick_}};
    row_(8, foot, 1);
}

}  // namespace bt
