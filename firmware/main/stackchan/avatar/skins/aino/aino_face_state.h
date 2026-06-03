/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * FaceState is the snapshot the renderer compositor reads each frame. The
 * Avatar's eye/mouth Feature stubs mutate this directly (e.g. the modifier
 * system calls leftEye().setWeight(N) which forwards to FaceState).
 *
 * Animation is just a sequence of FaceState mutations over time; no separate
 * animation buffers needed.
 */
#pragma once
#include "aino_palette.h"
#include "aino_sprites.h"
#include <cstdint>

namespace stackchan::avatar::aino {

// Aino-specific extended emotion set. The base Emotion enum only carries the
// six original states; we widen this internally for the four new states and
// the Blink intermediate.
enum class AinoState : uint8_t {
    Neutral = 0,
    Happy,
    Angry,
    Sad,
    Doubt,
    Sleepy,
    Excited,
    Love,
    Dead,
    Blink,
};

// Per-eye sub-state — selected by the renderer based on FaceState + weight.
// Weight semantics (matching the base Feature contract): 0 = closed, 100 = open.
enum class EyeShape : uint8_t {
    PILL,        // weight ~100, neutral round
    PILL_75,     // weight ~75
    PILL_50,     // weight ~50 (squint)
    BAR_TALL,    // weight ~30, intermediate close
    BAR,         // weight ~15
    DASH,        // weight ~0 (fully closed)
    HAPPY,       // emotion override: caret-up
    SAD,         // caret-down
    EYE_ANG,     // angry wedge
    EYE_ANG_M,   // mirrored angry wedge
    RING,        // excited ring
    HEART,       // love
    XEYE,        // dead X
    DBROW,       // doubt brow (right eye only when in Doubt)
    HIDDEN,      // skip drawing
};

enum class MouthShape : uint8_t {
    NONE,        // mouthless (Aino default)
    FLAT,        // small dash
    OPEN,        // talking-loud open
    FROWN,       // angry frown
};

// Master snapshot. Everything the renderer needs to draw one frame.
struct FaceState {
    AinoState state          = AinoState::Neutral;
    Palette   palette        = palette::yellow;

    EyeShape  leftShape      = EyeShape::PILL;
    EyeShape  rightShape     = EyeShape::PILL;
    MouthShape mouth         = MouthShape::NONE;

    // Eye position offset in device pixels (saccade / look-around).
    // Applied identically to both eyes; for asymmetric look use leftOffset/rightOffset.
    int16_t eyeOffsetX       = 0;
    int16_t eyeOffsetY       = 0;

    // Decorator sprites — optional accessories that get drawn alongside.
    // For Sad-state teardrop, Angry-state CROSS, Sleepy zzz sequence.
    struct Decoration {
        const Sprite* sprite = nullptr;
        int16_t ccol         = 0;   // center column (art units)
        int16_t trow         = 0;   // top row (art units)
        bool mirror          = false;
        uint8_t intensity    = 255; // 0..255 for fade
    };
    Decoration decos[6];     // Up to 6 simultaneous decorations
    uint8_t numDecos         = 0;

    // Bloom multiplier — animated for breathing pulse, set ~0.5..1.5
    float bloomScale         = 1.0f;

    // Generation counter — bump whenever any field changes so renderer knows
    // to repaint. Avoids per-frame full diff.
    uint32_t generation      = 0;
};

}  // namespace stackchan::avatar::aino
