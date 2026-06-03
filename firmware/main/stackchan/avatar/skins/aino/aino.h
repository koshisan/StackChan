/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Aino-bot avatar skin. Owns a FaceState + FaceRenderer; the eye/mouth Feature
 * stubs forward setWeight/setEmotion etc. to FaceState so the upstream
 * Modifier system (blink, speaking, idle saccade, ...) Just Works.
 */
#pragma once
#include "../../avatar/avatar.h"
#include "../../avatar/elements/feature.h"
#include "../../avatar/elements/speech_bubble.h"
#include "aino_face_state.h"
#include "aino_render.h"
#include <lvgl.h>
#include <smooth_lvgl.hpp>
#include <memory>

namespace stackchan::avatar {

namespace aino {
class AinoEye;
class AinoMouth;
class AinoSpeechBubble;
}

class AinoAvatar : public Avatar {
public:
    void init(lv_obj_t* parent, const lv_font_t* font = &lv_font_montserrat_16);

    /** Override base setEmotion to re-apply the recipe + bump generation. */
    void setEmotion(const Emotion& emotion) override;

    /** Called every UI tick — re-renders if generation advanced. */
    void update() override;

    aino::FaceState& faceState() { return _state; }
    aino::FaceRenderer& renderer() { return _renderer; }

    lv_obj_t* getCanvas() const { return _renderer.getCanvas(); }

    /** Same shape as DefaultAvatar::getPanel() — call sites swap transparently. */
    uitk::lvgl_cpp::Container* getPanel() const { return _pannel.get(); }

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _pannel;
    aino::FaceState   _state;
    aino::FaceRenderer _renderer;
};

namespace aino {

// Eye stub. Modifier system calls setWeight/setEmotion on these; we translate
// the weight ramp into an EyeShape and stash it in the parent FaceState.
class AinoEye : public ::stackchan::avatar::Feature {
public:
    AinoEye(FaceState* shared, bool isLeft);

    void setWeight(int weight) override;
    void setEmotion(const Emotion& emotion) override;
    void setVisible(bool visible) override;
    void setSize(int /*size*/) override {}
    void setRotation(int /*rotation*/) override {}
    void setPosition(const uitk::Vector2i& position) override;

private:
    FaceState* _shared = nullptr;
    bool _isLeft       = true;
};

class AinoMouth : public ::stackchan::avatar::Feature {
public:
    explicit AinoMouth(FaceState* shared);

    void setWeight(int weight) override;
    void setVisible(bool visible) override;
    void setRotation(int /*rotation*/) override {}
    void setPosition(const uitk::Vector2i& /*position*/) override {}

private:
    FaceState* _shared = nullptr;
};

class AinoSpeechBubble : public ::stackchan::avatar::SpeechBubble {
public:
    AinoSpeechBubble(lv_obj_t* parent, const lv_font_t* font);

    void setSpeech(std::string_view text) override;
    void clearSpeech() override;
    void setVisible(bool visible) override;
    void setTextFont(void* font) override;

private:
    std::unique_ptr<uitk::lvgl_cpp::Container> _bubble;
    std::unique_ptr<uitk::lvgl_cpp::Label> _text;
};

}  // namespace aino

}  // namespace stackchan::avatar
