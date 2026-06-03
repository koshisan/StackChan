/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 */
#include "aino.h"
#include "aino_states.h"

#include <esp_log.h>
#include <algorithm>

using namespace uitk::lvgl_cpp;
using namespace stackchan::avatar;
using namespace stackchan::avatar::aino;

// ============================================================================
// AinoAvatar
// ============================================================================

void AinoAvatar::init(lv_obj_t* parent, const lv_font_t* font) {
    ESP_LOGI("aino-init", "AinoAvatar::init parent=%p", parent);

    _pannel = std::make_unique<Container>(parent);
    _pannel->align(LV_ALIGN_CENTER, 0, 0);
    _pannel->setSize(DISP_W, DISP_H);
    _pannel->setRadius(0);
    _pannel->setBorderWidth(0);
    _pannel->setBgColor(lv_color_black());
    _pannel->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    _composer.init(_pannel->get());

    // Stub Features that mutate _state — modifier system goes through these.
    _key_elements.leftEye  = std::make_unique<AinoEye>(&_state, /*isLeft=*/true);
    _key_elements.rightEye = std::make_unique<AinoEye>(&_state, /*isLeft=*/false);
    _key_elements.mouth    = std::make_unique<AinoMouth>(&_state);
    _key_elements.speechBubble = std::make_unique<AinoSpeechBubble>(_pannel->get(), font);

    applyEmotionRecipe(_state, Emotion::Neutral);
    _composer.render(_state);
}

void AinoAvatar::setEmotion(const Emotion& emotion) {
    _emotion = emotion;
    applyEmotionRecipe(_state, emotion);
    // Base class also informs decorator pool — keep that behaviour.
    _decorator_pool.forEach([&emotion](Decorator* decorator, int /*id*/) {
        decorator->setEmotion(emotion);
    });
}

void AinoAvatar::update() {
    // Don't call base update — we don't drive children as separate LVGL objects.
    _decorator_pool.forEach([](Decorator* decorator, int /*id*/) {
        decorator->_update();
    });
    _decorator_pool.cleanup();

    _composer.render(_state);
}

// ============================================================================
// AinoEye — forwards modifier setWeight() into FaceState.{left,right}Shape
// ============================================================================

namespace stackchan::avatar::aino {

AinoEye::AinoEye(FaceState* shared, bool isLeft) : _shared(shared), _isLeft(isLeft) {}

void AinoEye::setWeight(int weight) {
    Feature::setWeight(weight);   // base clamps to 0..100
    if (!_shared) return;

    // If the current emotion has a non-PILL eye shape (Happy, Angry, Love, ...),
    // weight only matters for blink-style overrides — i.e., low weight forces
    // the eye closed regardless. Otherwise we map the weight ramp onto
    // PILL → DASH progression.

    EyeShape& slot = _isLeft ? _shared->leftShape : _shared->rightShape;

    // Detect whether the current shape is "pill-family" (responsive to weight)
    // or a hard emotion shape (responds only to extreme close).
    auto isPillFamily = [](EyeShape s) {
        return s == EyeShape::PILL || s == EyeShape::PILL_75 ||
               s == EyeShape::PILL_50 || s == EyeShape::BAR_TALL ||
               s == EyeShape::BAR || s == EyeShape::DASH;
    };

    if (isPillFamily(slot)) {
        EyeShape next;
        if      (weight >= 90) next = EyeShape::PILL;
        else if (weight >= 70) next = EyeShape::PILL_75;
        else if (weight >= 45) next = EyeShape::PILL_50;
        else if (weight >= 25) next = EyeShape::BAR_TALL;
        else if (weight >= 8 ) next = EyeShape::BAR;
        else                   next = EyeShape::DASH;
        if (next != slot) {
            slot = next;
            ++_shared->generation;
        }
    } else {
        // Hard emotion shape: only react to blink (weight ~0 → DASH override).
        if (weight < 15) {
            slot = EyeShape::DASH;
            ++_shared->generation;
        }
    }
}

void AinoEye::setEmotion(const Emotion& /*emotion*/) {
    // Avatar::setEmotion drives FaceState directly via applyEmotionRecipe.
    // Element-level setEmotion would otherwise overwrite our recipe — make it a no-op.
}

void AinoEye::setVisible(bool visible) {
    Element::setVisible(visible);
    if (!_shared) return;
    EyeShape& slot = _isLeft ? _shared->leftShape : _shared->rightShape;
    if (!visible) {
        slot = EyeShape::HIDDEN;
        ++_shared->generation;
    }
}

void AinoEye::setPosition(const uitk::Vector2i& position) {
    Element::setPosition(position);
    if (!_shared) return;
    // Normalized -100..100 → device px. Magic 0.5 just keeps motion subtle (max ±50px).
    int dx = position.x / 2;
    int dy = position.y / 2;
    if (_shared->eyeOffsetX != dx || _shared->eyeOffsetY != dy) {
        _shared->eyeOffsetX = (int16_t)dx;
        _shared->eyeOffsetY = (int16_t)dy;
        ++_shared->generation;
    }
}

// ============================================================================
// AinoMouth
// ============================================================================

AinoMouth::AinoMouth(FaceState* shared) : _shared(shared) {}

void AinoMouth::setWeight(int weight) {
    Feature::setWeight(weight);
    if (!_shared) return;
    MouthShape next;
    if      (weight >= 60) next = MouthShape::OPEN;
    else if (weight >= 20) next = MouthShape::FLAT;
    else                   next = MouthShape::NONE;
    if (next != _shared->mouth) {
        _shared->mouth = next;
        ++_shared->generation;
    }
}

void AinoMouth::setVisible(bool visible) {
    Element::setVisible(visible);
    if (!_shared) return;
    if (!visible && _shared->mouth != MouthShape::NONE) {
        _shared->mouth = MouthShape::NONE;
        ++_shared->generation;
    }
}

// ============================================================================
// AinoSpeechBubble — minimal Aino-styled bubble (still uses default geometry)
// ============================================================================

AinoSpeechBubble::AinoSpeechBubble(lv_obj_t* parent, const lv_font_t* font) {
    _bubble = std::make_unique<Container>(parent);
    _bubble->align(LV_ALIGN_BOTTOM_MID, 0, -6);
    _bubble->setSize(LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    _bubble->setRadius(10);
    _bubble->setBgColor(lv_color_make(0, 0, 0));
    _bubble->setBorderWidth(1);
    _bubble->setBorderColor(palette::yellow.fg);
    _bubble->removeFlag(LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_bubble->get(), LV_OBJ_FLAG_HIDDEN);

    _text = std::make_unique<Label>(_bubble->get());
    _text->setTextFont(font);
    _text->setTextColor(palette::yellow.fg);
    _text->setText("");
}

void AinoSpeechBubble::setSpeech(std::string_view text) {
    if (!_text || !_bubble) return;
    _text->setText(std::string(text).c_str());
    lv_obj_remove_flag(_bubble->get(), LV_OBJ_FLAG_HIDDEN);
}

void AinoSpeechBubble::clearSpeech() {
    if (!_text || !_bubble) return;
    _text->setText("");
    lv_obj_add_flag(_bubble->get(), LV_OBJ_FLAG_HIDDEN);
}

void AinoSpeechBubble::setVisible(bool visible) {
    Element::setVisible(visible);
    if (!_bubble) return;
    if (visible) {
        lv_obj_remove_flag(_bubble->get(), LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(_bubble->get(), LV_OBJ_FLAG_HIDDEN);
    }
}

void AinoSpeechBubble::setTextFont(void* font) {
    if (_text) _text->setTextFont(static_cast<const lv_font_t*>(font));
}

}  // namespace stackchan::avatar::aino
