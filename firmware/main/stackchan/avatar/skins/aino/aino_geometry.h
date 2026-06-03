/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Geometry constants — translated from the design HTML. All values are in
 * device pixels (320×240).
 */
#pragma once
#include <cstdint>

namespace stackchan::avatar::aino {

inline constexpr int DISP_W = 320;
inline constexpr int DISP_H = 240;

// Visible face/screen region. The Aino design HTML specified a 240×180 lit
// area on a 320×240 panel with a black "robot housing" bezel — but on the
// actual M5Stack Core2 display the case ALREADY provides a plastic bezel,
// and shrinking the lit area further makes the face look small and lost.
// So: lit area now fills the full panel, corners stay slightly rounded.
inline constexpr int FACE_W = 320;
inline constexpr int FACE_H = 240;
inline constexpr int FACE_X = 0;
inline constexpr int FACE_Y = 0;
inline constexpr int FACE_CX = 160;
inline constexpr int FACE_CY = 120;
inline constexpr int FACE_R  = 12;      // mild CRT corner

// Sprite cell (art-pixel) size in device pixels.
inline constexpr int ART = 8;

// Grid origin (in device px). GX = FACE_X so cells align to face edge.
// GY = 32 is a multiple of ART so the row-mask lands on cell edges.
inline constexpr int GX = 40;
inline constexpr int GY = 32;

// Eye centre columns (in art units).
inline constexpr int EYE_L = 9;     // device x = GX + 9*ART = 112
inline constexpr int EYE_R = 21;    // device x = GX + 21*ART = 208

// Vignette tunables (from design — Bloom 0.5, Vignette dim 0.93, edge 0.25).
inline constexpr float BLOOM_SCALE  = 0.5f;
inline constexpr float VIGNETTE_DIM = 0.93f;
inline constexpr float VIGNETTE_EDGE = 0.25f;

}  // namespace stackchan::avatar::aino
