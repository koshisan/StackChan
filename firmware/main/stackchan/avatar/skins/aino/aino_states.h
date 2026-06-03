/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Per-emotion FaceState recipes. Maps the public Emotion enum to:
 *  - one of the AinoState extended values
 *  - the palette
 *  - the left/right eye shapes
 *  - the default mouth shape (NONE for Aino-true look)
 *  - any persistent decorations (angry CROSS, sad teardrop, sleepy zzz)
 *
 * Animation modifiers (blink, speaking, saccade) layer on top by mutating
 * FaceState fields; the recipe is the "rest state" for a given emotion.
 */
#pragma once
#include "aino_face_state.h"
#include "../../avatar/elements/emotion.h"

namespace stackchan::avatar::aino {

void applyEmotionRecipe(FaceState& fs, Emotion e);

// AinoState versions for the four extended emotions — these can be triggered
// by future code that wants Excited/Love/Dead/Blink without going through the
// six-value Emotion enum.
void applyAinoState(FaceState& fs, AinoState s);

}  // namespace stackchan::avatar::aino
