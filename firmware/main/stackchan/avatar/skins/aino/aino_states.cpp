/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 */
#include "aino_states.h"

namespace stackchan::avatar::aino {

// Decoration helpers.
static void addDeco(FaceState& fs, const Sprite& s, int ccol, int trow, bool mirror = false) {
    if (fs.numDecos >= (uint8_t)(sizeof(fs.decos) / sizeof(fs.decos[0]))) return;
    auto& d = fs.decos[fs.numDecos++];
    d.sprite = &s;
    d.ccol = ccol;
    d.trow = trow;
    d.mirror = mirror;
    d.intensity = 255;
}

static void resetDecos(FaceState& fs) {
    fs.numDecos = 0;
}

void applyAinoState(FaceState& fs, AinoState s) {
    fs.state = s;
    resetDecos(fs);
    fs.eyeOffsetX = 0;
    fs.eyeOffsetY = 0;
    fs.bloomScale = 1.0f;

    switch (s) {
        case AinoState::Neutral:
            fs.palette    = palette::yellow;
            fs.leftShape  = EyeShape::PILL;
            fs.rightShape = EyeShape::PILL;
            fs.mouth      = MouthShape::FLAT;   // talking-state default; modifier can clear
            break;

        case AinoState::Happy:
            fs.palette    = palette::yellow;
            fs.leftShape  = EyeShape::HAPPY;
            fs.rightShape = EyeShape::HAPPY;
            fs.mouth      = MouthShape::FLAT;
            break;

        case AinoState::Angry:
            fs.palette    = palette::red;
            fs.leftShape  = EyeShape::EYE_ANG;
            fs.rightShape = EyeShape::EYE_ANG_M;
            fs.mouth      = MouthShape::FROWN;
            addDeco(fs, CROSS, /*ccol=*/24, /*trow=*/1);    // anger vein top-right
            break;

        case AinoState::Sad:
            fs.palette    = palette::yellow;
            fs.leftShape  = EyeShape::SAD;
            fs.rightShape = EyeShape::SAD;
            fs.mouth      = MouthShape::FLAT;
            addDeco(fs, DOT, /*ccol=*/24, /*trow=*/12);     // teardrop
            break;

        case AinoState::Doubt:
            fs.palette    = palette::yellow;
            fs.leftShape  = EyeShape::PILL;
            fs.rightShape = EyeShape::BAR_TALL;             // squinted right eye
            fs.mouth      = MouthShape::NONE;
            addDeco(fs, DBROW, /*ccol=*/EYE_R, /*trow=*/6); // raised brow on right
            break;

        case AinoState::Sleepy:
            fs.palette    = palette::amber;
            fs.leftShape  = EyeShape::DASH;
            fs.rightShape = EyeShape::DASH;
            fs.mouth      = MouthShape::NONE;
            // Rising zzz sequence
            addDeco(fs, CELL, 25, 8);
            addDeco(fs, CELL, 26, 6);
            addDeco(fs, CELL, 27, 4);
            break;

        case AinoState::Excited:
            fs.palette    = palette::bright;
            fs.leftShape  = EyeShape::RING;
            fs.rightShape = EyeShape::RING;
            fs.mouth      = MouthShape::NONE;
            fs.bloomScale = 1.3f;                            // extra glow when excited
            break;

        case AinoState::Love:
            fs.palette    = palette::pink;
            fs.leftShape  = EyeShape::HEART;
            fs.rightShape = EyeShape::HEART;
            fs.mouth      = MouthShape::NONE;
            break;

        case AinoState::Dead:
            fs.palette    = palette::amber;
            fs.leftShape  = EyeShape::XEYE;
            fs.rightShape = EyeShape::XEYE;
            fs.mouth      = MouthShape::FLAT;
            fs.bloomScale = 0.7f;                            // dimmer "low battery" look
            break;

        case AinoState::Blink:
            // Caller usually doesn't drive Blink as a stable state — animation
            // engine flicks through it. We still support it for raw control.
            fs.palette    = palette::yellow;
            fs.leftShape  = EyeShape::DASH;
            fs.rightShape = EyeShape::DASH;
            fs.mouth      = MouthShape::FLAT;
            break;
    }
    ++fs.generation;
}

void applyEmotionRecipe(FaceState& fs, Emotion e) {
    AinoState s = AinoState::Neutral;
    switch (e) {
        case Emotion::Neutral: s = AinoState::Neutral; break;
        case Emotion::Happy:   s = AinoState::Happy;   break;
        case Emotion::Angry:   s = AinoState::Angry;   break;
        case Emotion::Sad:     s = AinoState::Sad;     break;
        case Emotion::Doubt:   s = AinoState::Doubt;   break;
        case Emotion::Sleepy:  s = AinoState::Sleepy;  break;
    }
    applyAinoState(fs, s);
}

}  // namespace stackchan::avatar::aino
