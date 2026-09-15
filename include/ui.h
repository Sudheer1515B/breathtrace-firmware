// ILI9488 display — draws the same 20-column x 7-row grid the enclosure render
// shows, so the firmware output and the industrial-design mockup agree.
//
// Rows are cached and only redrawn when their content changes; a full redraw of
// a 480x320 SPI panel every loop would both flicker and starve the sensors.
#pragma once
#include <stdint.h>
#include "classifier.h"
#include "config.h"

namespace bt {

struct Seg {
    int col;             // 0-19, character column
    const char* text;
    uint16_t color;
};

class Ui {
public:
    void begin();
    void set_clock(const char* hhmm);     // "14:32", or "--:--" with no time source

    void screen_idle(int battery_pct);
    void screen_progress(const char* label, const char* detail, int pct);
    void screen_collecting(int breaths, int required, int co2_ppm, uint32_t elapsed_s);
    void screen_result(const PanelResult results[PANEL_COUNT], int co2_mean_ppm,
                       int breaths);
    void screen_invalid(const char* reason);
    void screen_fault(const char* reason);

    // Palette, exposed so other modules can match it.
    uint16_t orange() const { return c_orange_; }
    uint16_t brick() const  { return c_brick_; }
    uint16_t teal() const   { return c_teal_; }
    uint16_t white() const  { return c_white_; }
    uint16_t dim() const    { return c_dim_; }

private:
    void row_(int index, const Seg* segs, int n);
    void rule_(int index);
    void invalidate_();                   // force the next paint to redraw all

    char cache_[9][21] = {};
    uint16_t cache_col_[9][20] = {};
    bool cache_valid_[9] = {};

    char clock_[8] = {'-', '-', ':', '-', '-', 0, 0, 0};
    uint16_t c_orange_ = 0, c_brick_ = 0, c_teal_ = 0, c_white_ = 0, c_dim_ = 0,
             c_rule_ = 0, c_bg_ = 0;
};

}  // namespace bt
