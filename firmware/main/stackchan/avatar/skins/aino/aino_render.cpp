/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 */
#include "aino_render.h"
#include <esp_log.h>

namespace stackchan::avatar::aino {

static const char* TAG = "aino-render";

FaceComposer::~FaceComposer() {
    // Children are auto-cleaned by LVGL when the parent is destroyed.
}

void FaceComposer::init(lv_obj_t* parent) {
    _parent = parent;

    // ----- Face container: the lit "screen" area -----
    _face = lv_obj_create(parent);
    lv_obj_set_size(_face, FACE_W, FACE_H);
    lv_obj_set_pos(_face, FACE_X, FACE_Y);
    lv_obj_set_style_radius(_face, FACE_R, 0);
    lv_obj_set_style_border_width(_face, 0, 0);
    lv_obj_set_style_bg_color(_face, palette::yellow.bg, 0);
    lv_obj_set_style_pad_all(_face, 0, 0);
    lv_obj_clear_flag(_face, LV_OBJ_FLAG_SCROLLABLE);

    // ----- Row-mask strips: odd ART-rows get bgDim -----
    // FACE_H / ART = 180/8 = 22 rows; ~11 odd rows.
    int rowsInFace = FACE_H / ART;
    int firstRow = (FACE_Y - GY) / ART;
    for (int r = 0; r < rowsInFace; ++r) {
        int rowIdxFromGrid = firstRow + r;
        if (!(rowIdxFromGrid & 1)) continue;
        lv_obj_t* strip = lv_obj_create(_face);
        lv_obj_set_size(strip, FACE_W, ART);
        lv_obj_set_pos(strip, 0, r * ART);
        lv_obj_set_style_radius(strip, 0, 0);
        lv_obj_set_style_border_width(strip, 0, 0);
        lv_obj_set_style_bg_color(strip, palette::yellow.bgDim, 0);
        lv_obj_set_style_pad_all(strip, 0, 0);
        lv_obj_clear_flag(strip, LV_OBJ_FLAG_SCROLLABLE);
        _maskStrips.push_back(strip);
    }

    ESP_LOGI(TAG, "FaceComposer init: face=%p mask=%d strips", _face, (int)_maskStrips.size());
}

void FaceComposer::ensureCellCount(size_t needed) {
    while (_cells.size() < needed) {
        lv_obj_t* cell = lv_obj_create(_face);
        lv_obj_set_size(cell, ART, ART);
        lv_obj_set_style_radius(cell, 0, 0);
        lv_obj_set_style_border_width(cell, 0, 0);
        lv_obj_set_style_pad_all(cell, 0, 0);
        lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
        // Phosphor bloom approximation: per-cell shadow with no x/y offset
        // gives a warm halo around the lit pixel. Spread keeps the inner
        // ring solid; width softens the outer edge. Opa kept moderate so
        // overlapping shadows don't go saturated when sprite cells touch.
        lv_obj_set_style_shadow_width(cell, 12, 0);
        lv_obj_set_style_shadow_spread(cell, 2, 0);
        lv_obj_set_style_shadow_offset_x(cell, 0, 0);
        lv_obj_set_style_shadow_offset_y(cell, 0, 0);
        lv_obj_set_style_shadow_opa(cell, LV_OPA_60, 0);
        lv_obj_add_flag(cell, LV_OBJ_FLAG_HIDDEN);
        _cells.push_back(cell);
    }
}

void FaceComposer::hideRemainingCells(size_t fromIdx) {
    for (size_t i = fromIdx; i < _cells.size(); ++i) {
        lv_obj_add_flag(_cells[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void FaceComposer::retintBackground(const Palette& p) {
    lv_obj_set_style_bg_color(_face, p.bg, 0);
    for (auto* strip : _maskStrips) {
        lv_obj_set_style_bg_color(strip, p.bgDim, 0);
    }
}

void FaceComposer::drawSpriteCells(const Sprite& s, int ccol, int trow,
                                   bool mirror, const Palette& p, size_t& cellIdx) {
    // Position is in _face's local coordinate system. Subtract FACE_X/FACE_Y
    // from absolute grid coordinates to get face-local.
    const float c0f = ccol - (s.w / 2.0f);
    const int c0   = (int)c0f;
    for (int r = 0; r < s.h; ++r) {
        int gridRow = trow + r;
        bool darkRow = (gridRow & 1);
        lv_color_t col = darkRow ? p.fgDim : p.fg;
        int absY = GY + gridRow * ART;
        int yLocal = absY - FACE_Y;
        for (int c = 0; c < s.w; ++c) {
            int srcCol = mirror ? (s.w - 1 - c) : c;
            if (!spriteBit(s, r, srcCol)) continue;
            int absX = GX + (c0 + c) * ART;
            int xLocal = absX - FACE_X;
            // Clip outside face area
            if (xLocal < 0 || xLocal + ART > FACE_W) continue;
            if (yLocal < 0 || yLocal + ART > FACE_H) continue;
            ensureCellCount(cellIdx + 1);
            lv_obj_t* cell = _cells[cellIdx++];
            lv_obj_set_pos(cell, xLocal, yLocal);
            lv_obj_set_style_bg_color(cell, col, 0);
            // Bloom halo: match shadow color to the fg tone of this row's parity.
            lv_obj_set_style_shadow_color(cell, p.fg, 0);
            lv_obj_clear_flag(cell, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void FaceComposer::render(const FaceState& fs) {
    if (!_face) return;
    if (fs.generation == _lastGen) return;
    _lastGen = fs.generation;

    retintBackground(fs.palette);

    size_t cellIdx = 0;

    // ----- Eyes -----
    struct EyeRecipe { const Sprite* s; int trow; bool mirror; };
    auto eyeRecipe = [](EyeShape sh) -> EyeRecipe {
        switch (sh) {
            case EyeShape::PILL:      return { &PILL,      5, false };
            case EyeShape::PILL_75:   return { &PILL_75,   5, false };
            case EyeShape::PILL_50:   return { &PILL_50,   5, false };
            case EyeShape::BAR_TALL:  return { &BAR_TALL,  7, false };
            case EyeShape::BAR:       return { &BAR,       9, false };
            case EyeShape::DASH:      return { &DASH,      9, false };
            case EyeShape::HAPPY:     return { &HAPPY,     6, false };
            case EyeShape::SAD:       return { &SAD,       7, false };
            case EyeShape::EYE_ANG:   return { &EYE_ANG,  10, false };
            case EyeShape::EYE_ANG_M: return { &EYE_ANG,  10, true  };
            case EyeShape::RING:      return { &RING,      6, false };
            case EyeShape::HEART:     return { &HEART,     6, false };
            case EyeShape::XEYE:      return { &XEYE,      6, false };
            case EyeShape::DBROW:     return { &DBROW,     6, false };
            case EyeShape::HIDDEN:    return { nullptr,    0, false };
        }
        return { nullptr, 0, false };
    };

    int colOff = fs.eyeOffsetX / ART;
    int rowOff = fs.eyeOffsetY / ART;

    EyeRecipe lr = eyeRecipe(fs.leftShape);
    if (lr.s) drawSpriteCells(*lr.s, EYE_L + colOff, lr.trow + rowOff, lr.mirror, fs.palette, cellIdx);

    EyeRecipe rr = eyeRecipe(fs.rightShape);
    if (rr.s) drawSpriteCells(*rr.s, EYE_R + colOff, rr.trow + rowOff, rr.mirror, fs.palette, cellIdx);

    // ----- Mouth -----
    const Sprite* m = nullptr;
    int mtrow = 15;
    switch (fs.mouth) {
        case MouthShape::NONE:  m = nullptr; break;
        case MouthShape::FLAT:  m = &MFLAT;  mtrow = 15; break;
        case MouthShape::OPEN:  m = &MOPEN;  mtrow = 14; break;
        case MouthShape::FROWN: m = &MFROWN; mtrow = 14; break;
    }
    if (m) drawSpriteCells(*m, 15, mtrow, false, fs.palette, cellIdx);

    // ----- Decorations -----
    for (int i = 0; i < fs.numDecos; ++i) {
        const auto& d = fs.decos[i];
        if (!d.sprite) continue;
        drawSpriteCells(*d.sprite, d.ccol, d.trow, d.mirror, fs.palette, cellIdx);
    }

    hideRemainingCells(cellIdx);
}

}  // namespace stackchan::avatar::aino
