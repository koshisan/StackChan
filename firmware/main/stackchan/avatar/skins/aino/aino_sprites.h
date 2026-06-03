/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Aino-bot face sprite library — translated from the design HTML's pixel-art
 * tables. Each sprite is a width × height matrix of bits packed into uint8_t
 * rows, MSB-first (so a row of width 6 fits in the top 6 bits of one byte).
 *
 * Layout: rows[h] each holding `w` bits, bit (1 << (w-1-col)) lit.
 *
 * The renderer iterates rows × cols, blits an ART-px (=8 device-px) cell for
 * every lit bit at (grid_x + (col+c0)*ART, grid_y + (row+trow)*ART).
 *
 * Variants used by the animation engine (PILL_75/50/25, BAR_TALL, etc.) cover
 * blink / squint intermediate frames the static design didn't show.
 */
#pragma once
#include <cstdint>
#include <cstddef>

namespace stackchan::avatar::aino {

struct Sprite {
    uint8_t w;          // width in art-cells (1..8)
    uint8_t h;          // height in art-cells (1..8)
    const uint8_t* rows; // h bytes; bit (1<<(w-1-col)) lit
};

// Bit-packed row literal: bit b7 = col 0. Use only top `w` bits.
// (We write rows MSB-first so the literal reads left-to-right like ASCII art.)
namespace _bits {

// PILL  .##. ####  ####  ####  ####  ####  ####  .##.   (4 wide, 8 tall)
inline constexpr uint8_t PILL_rows[] = {
    0b01100000, 0b11110000, 0b11110000, 0b11110000,
    0b11110000, 0b11110000, 0b11110000, 0b01100000,
};

// PILL_75 — top + bottom rows clipped (eye 75% open)
inline constexpr uint8_t PILL_75_rows[] = {
    0b00000000, 0b11110000, 0b11110000, 0b11110000,
    0b11110000, 0b11110000, 0b11110000, 0b00000000,
};

// PILL_50 — top three rows clipped (eye 50% — slits)
inline constexpr uint8_t PILL_50_rows[] = {
    0b00000000, 0b00000000, 0b00000000, 0b11110000,
    0b11110000, 0b00000000, 0b00000000, 0b00000000,
};

// HAPPY ^ caret (8 wide, 4 tall)
inline constexpr uint8_t HAPPY_rows[] = {
    0b00011000, 0b00111100, 0b01100110, 0b11000011,
};

// SAD v caret
inline constexpr uint8_t SAD_rows[] = {
    0b11000011, 0b01100110, 0b00111100, 0b00011000,
};

// EYE_ANG — angry wedge (7 wide, 8 tall, fills SW corner with rounded outer-bottom)
inline constexpr uint8_t EYE_ANG_rows[] = {
    0b11000000, 0b11000000, 0b11100000, 0b11100000,
    0b11110000, 0b11111000, 0b11111100, 0b01111110,
};

// CROSS — anger vein "+" (7 wide, 7 tall, 1-row center gap)
inline constexpr uint8_t CROSS_rows[] = {
    0b00101000, 0b00101000, 0b11101110, 0b00000000,
    0b11101110, 0b00101000, 0b00101000,
};

// RING — excited closed ring (6×6)
inline constexpr uint8_t RING_rows[] = {
    0b01111000, 0b11001100, 0b11001100, 0b11001100,
    0b11001100, 0b01111000,
};

// HEART — love (8×7)
inline constexpr uint8_t HEART_rows[] = {
    0b01100110, 0b11111111, 0b11111111, 0b11111111,
    0b01111110, 0b00111100, 0b00011000,
};

// XEYE — dead "X" (6×6)
inline constexpr uint8_t XEYE_rows[] = {
    0b10000100, 0b01001000, 0b00110000, 0b00110000,
    0b01001000, 0b10000100,
};

// DASH — sleepy / blink lid (6×2)
inline constexpr uint8_t DASH_rows[] = {
    0b11111100, 0b11111100,
};

// BAR — squint (4×2)
inline constexpr uint8_t BAR_rows[] = {
    0b11110000, 0b11110000,
};

// BAR_TALL — 4×4, intermediate close
inline constexpr uint8_t BAR_TALL_rows[] = {
    0b11110000, 0b11110000, 0b11110000, 0b11110000,
};

// DBROW — doubt brow (6×1)
inline constexpr uint8_t DBROW_rows[] = {
    0b01111000,
};

// MFLAT — small mouth dash (8×1)
inline constexpr uint8_t MFLAT_rows[] = {
    0b01111110,
};

// MOPEN — mouth open (talking-loud) (8×3 oval-ish)
inline constexpr uint8_t MOPEN_rows[] = {
    0b00111100, 0b01111110, 0b00111100,
};

// MFROWN — angry frown (8×2)
inline constexpr uint8_t MFROWN_rows[] = {
    0b00111100, 0b01100110,
};

// DOT — pupil / teardrop (2×2)
inline constexpr uint8_t DOT_rows[] = {
    0b11000000, 0b11000000,
};

// CELL — single zzz cell (1×1)
inline constexpr uint8_t CELL_rows[] = {
    0b10000000,
};

}  // namespace _bits

// Public sprite handles. Keep names parallel to the design HTML.
inline constexpr Sprite PILL      = { 4, 8, _bits::PILL_rows };
inline constexpr Sprite PILL_75   = { 4, 8, _bits::PILL_75_rows };
inline constexpr Sprite PILL_50   = { 4, 8, _bits::PILL_50_rows };
inline constexpr Sprite HAPPY     = { 8, 4, _bits::HAPPY_rows };
inline constexpr Sprite SAD       = { 8, 4, _bits::SAD_rows };
inline constexpr Sprite EYE_ANG   = { 7, 8, _bits::EYE_ANG_rows };
inline constexpr Sprite CROSS     = { 7, 7, _bits::CROSS_rows };
inline constexpr Sprite RING      = { 6, 6, _bits::RING_rows };
inline constexpr Sprite HEART     = { 8, 7, _bits::HEART_rows };
inline constexpr Sprite XEYE      = { 6, 6, _bits::XEYE_rows };
inline constexpr Sprite DASH      = { 6, 2, _bits::DASH_rows };
inline constexpr Sprite BAR       = { 4, 2, _bits::BAR_rows };
inline constexpr Sprite BAR_TALL  = { 4, 4, _bits::BAR_TALL_rows };
inline constexpr Sprite DBROW     = { 6, 1, _bits::DBROW_rows };
inline constexpr Sprite MFLAT     = { 8, 1, _bits::MFLAT_rows };
inline constexpr Sprite MOPEN     = { 8, 3, _bits::MOPEN_rows };
inline constexpr Sprite MFROWN    = { 8, 2, _bits::MFROWN_rows };
inline constexpr Sprite DOT       = { 2, 2, _bits::DOT_rows };
inline constexpr Sprite CELL      = { 1, 1, _bits::CELL_rows };

// Bit test helper: lit at (row, col)?
constexpr bool spriteBit(const Sprite& s, int row, int col) {
    if (row < 0 || row >= s.h || col < 0 || col >= s.w) return false;
    return (s.rows[row] >> (7 - col)) & 1;
}

}  // namespace stackchan::avatar::aino
