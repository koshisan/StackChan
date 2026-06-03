/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Aino-bot face palettes — translated from the design HTML's COL constants.
 * Each palette has four sampled tones because the renderer paints alternating
 * bright/dark rows (the "CRT row-mask"). See design_handoff README.
 */
#pragma once
#include <lvgl.h>
#include <cstdint>

namespace stackchan::avatar::aino {

struct Palette {
    lv_color_t fg;      // sprite pixel on bright row
    lv_color_t fgDim;   // sprite pixel on dark row (~5-10% darker)
    lv_color_t bg;      // background on bright row
    lv_color_t bgDim;   // background on dark row (much darker)
};

// Helper: hex → lv_color_t. LVGL's lv_color_hex takes 0xRRGGBB.
constexpr lv_color_t HEX(uint32_t rgb) {
    return lv_color_make((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff);
}

namespace palette {

// yellow — default emission, used by Neutral / Happy / Sad / Doubt / Blink
inline constexpr Palette yellow = {
    HEX(0xfbcb21), HEX(0xefb817), HEX(0x431e0c), HEX(0x290d0a)
};

// bright — Excited, slightly warmer than yellow
inline constexpr Palette bright = {
    HEX(0xffd23e), HEX(0xf3c42b), HEX(0x48270d), HEX(0x2c1408)
};

// red — Angry
inline constexpr Palette red = {
    HEX(0xff3a2a), HEX(0xef2a1c), HEX(0x3d130b), HEX(0x260a08)
};

// pink — Love
inline constexpr Palette pink = {
    HEX(0xff5285), HEX(0xf53e74), HEX(0x3f1421), HEX(0x280b15)
};

// amber — Sleepy / Dead, dimmer than yellow
inline constexpr Palette amber = {
    HEX(0xe3a528), HEX(0xd4961d), HEX(0x3a1d0a), HEX(0x250f07)
};

}  // namespace palette

}  // namespace stackchan::avatar::aino
