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

// Visible face/screen region — everything outside is the plastic bezel (black).
inline constexpr int FACE_W = 240;
inline constexpr int FACE_H = 180;
inline constexpr int FACE_X = 40;       // FACE.X
inline constexpr int FACE_Y = 30;       // FACE.Y
inline constexpr int FACE_CX = 160;     // centre
inline constexpr int FACE_CY = 120;
inline constexpr int FACE_R  = 18;      // rounded-rect corner radius (CRT corner)

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
