/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 */
#include "aino_render.h"
#include "aino_glow.h"

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <algorithm>
#include <cstring>

namespace stackchan::avatar::aino {

static const char* TAG = "aino-render";

// We render directly into an RGB565 buffer. LVGL's lv_canvas wraps the same
// pointer so it can blit it to the parent at the next refresh.
static constexpr int CANVAS_W = DISP_W;
static constexpr int CANVAS_H = DISP_H;
static constexpr size_t CANVAS_BYTES = CANVAS_W * CANVAS_H * sizeof(lv_color_t);

FaceRenderer::~FaceRenderer() {
    if (_buf) {
        heap_caps_free(_buf);
        _buf = nullptr;
    }
}

void FaceRenderer::init(lv_obj_t* parent) {
    // Prefer PSRAM if available; ~150 KB buffer is uncomfortable in DRAM.
    _buf = static_cast<lv_color_t*>(
        heap_caps_aligned_alloc(64, CANVAS_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!_buf) {
        _buf = static_cast<lv_color_t*>(
            heap_caps_aligned_alloc(64, CANVAS_BYTES, MALLOC_CAP_8BIT));
    }
    if (!_buf) {
        ESP_LOGE(TAG, "canvas buffer alloc failed (%zu bytes)", CANVAS_BYTES);
        return;
    }

    _canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(_canvas, _buf, CANVAS_W, CANVAS_H, LV_COLOR_FORMAT_NATIVE);
    lv_obj_align(_canvas, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(_canvas, LV_OBJ_FLAG_SCROLLABLE);
}

// ---------------- primitives ----------------

inline void FaceRenderer::putPixel(int x, int y, lv_color_t c) {
    if ((unsigned)x >= CANVAS_W || (unsigned)y >= CANVAS_H) return;
    _buf[y * CANVAS_W + x] = c;
}

void FaceRenderer::fillRect(int x, int y, int w, int h, lv_color_t c) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(CANVAS_W, x + w);
    int y1 = std::min(CANVAS_H, y + h);
    for (int yy = y0; yy < y1; ++yy) {
        lv_color_t* row = _buf + yy * CANVAS_W;
        for (int xx = x0; xx < x1; ++xx) {
            row[xx] = c;
        }
    }
}

void FaceRenderer::fillFaceBackground(const Palette& p) {
    // Bezel first.
    fillRect(0, 0, CANVAS_W, CANVAS_H, lv_color_black());
    // Face area with bright-row bg.
    fillRect(FACE_X, FACE_Y, FACE_W, FACE_H, p.bg);
}

void FaceRenderer::applyRowMaskBackground(const Palette& p) {
    // Within the face rect, every odd ART-row (1, 3, 5, ...) becomes bgDim.
    // GY is multiple of ART so the band aligns to cell edges.
    int firstRow = (FACE_Y - GY) / ART;     // top row index of the face inside grid
    int rowsInFace = FACE_H / ART;          // 180/8 = 22 ART-rows
    for (int r = 0; r < rowsInFace; ++r) {
        int rowIdxFromGrid = firstRow + r;
        if (rowIdxFromGrid & 1) {           // odd → dark
            int y = GY + rowIdxFromGrid * ART;
            fillRect(FACE_X, y, FACE_W, ART, p.bgDim);
        }
    }
}

// Additive blend: out = clamp(out + (tint * alpha / 255)).
static inline lv_color_t addTint(lv_color_t base, lv_color_t tint, uint8_t alpha) {
    int br = lv_color_to_u32(base);
    int b_r = (br >> 16) & 0xff;
    int b_g = (br >> 8) & 0xff;
    int b_b = br & 0xff;
    int tr = lv_color_to_u32(tint);
    int t_r = (tr >> 16) & 0xff;
    int t_g = (tr >> 8) & 0xff;
    int t_b = tr & 0xff;
    int r = b_r + (t_r * alpha) / 255;
    int g = b_g + (t_g * alpha) / 255;
    int bb = b_b + (t_b * alpha) / 255;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (bb > 255) bb = 255;
    return lv_color_make((uint8_t)r, (uint8_t)g, (uint8_t)bb);
}

void FaceRenderer::blitGlow(int cx, int cy, const Palette& p, float scale) {
    // Center GLOW_ALPHA on (cx, cy). scale ∈ [0..2] modulates intensity.
    const int half = GLOW_SIZE / 2;
    const float k = BLOOM_SCALE * scale;
    for (int j = 0; j < GLOW_SIZE; ++j) {
        int y = cy - half + j;
        if ((unsigned)y >= CANVAS_H) continue;
        // Stay inside FACE rect — outside is bezel.
        if (y < FACE_Y || y >= FACE_Y + FACE_H) continue;
        for (int i = 0; i < GLOW_SIZE; ++i) {
            int x = cx - half + i;
            if ((unsigned)x >= CANVAS_W) continue;
            if (x < FACE_X || x >= FACE_X + FACE_W) continue;
            uint8_t a = GLOW_ALPHA[j * GLOW_SIZE + i];
            if (a == 0) continue;
            uint8_t aa = (uint8_t)std::min(255.0f, a * k);
            if (aa == 0) continue;
            lv_color_t& px = _buf[y * CANVAS_W + x];
            px = addTint(px, p.fg, aa);
        }
    }
}

void FaceRenderer::blitSpriteCells(const Sprite& s, int ccol, int trow,
                                   bool mirror, const Palette& p) {
    // Each lit cell is an 8×8 device-pixel block. Color depends on row parity:
    // bright row (even ART-row) → fg; dark row (odd) → fgDim.
    const float c0f = ccol - (s.w / 2.0f);
    const int c0 = (int)c0f;
    for (int r = 0; r < s.h; ++r) {
        int gridRow = trow + r;
        bool darkRow = (gridRow & 1);
        lv_color_t col = darkRow ? p.fgDim : p.fg;
        int y = GY + gridRow * ART;
        for (int c = 0; c < s.w; ++c) {
            int srcCol = mirror ? (s.w - 1 - c) : c;
            if (!spriteBit(s, r, srcCol)) continue;
            int x = GX + (c0 + c) * ART;
            fillRect(x, y, ART, ART, col);
        }
    }
}

void FaceRenderer::drawSprite(const Sprite& s, int ccol, int trow,
                              bool mirror, const Palette& p) {
    // 1) Bloom pass — one glow blob per lit cell, centered.
    const float c0f = ccol - (s.w / 2.0f);
    const int c0 = (int)c0f;
    for (int r = 0; r < s.h; ++r) {
        int y = GY + (trow + r) * ART + ART / 2;
        for (int c = 0; c < s.w; ++c) {
            int srcCol = mirror ? (s.w - 1 - c) : c;
            if (!spriteBit(s, r, srcCol)) continue;
            int x = GX + (c0 + c) * ART + ART / 2;
            blitGlow(x, y, p, 1.0f);
        }
    }
    // 2) Hard cells with row-mask tone choice.
    blitSpriteCells(s, ccol, trow, mirror, p);
}

void FaceRenderer::drawEyes(const FaceState& fs) {
    struct EyeRecipe { const Sprite* s; int trow; bool mirror; };
    auto eyeRecipe = [&](EyeShape sh, bool right) -> EyeRecipe {
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

    // Pixel offset → art column shift in 1-cell increments only (8px).
    int colOffL = fs.eyeOffsetX / ART;
    int colOffR = colOffL;
    int rowOff  = fs.eyeOffsetY / ART;

    EyeRecipe lr = eyeRecipe(fs.leftShape, false);
    if (lr.s) drawSprite(*lr.s, EYE_L + colOffL, lr.trow + rowOff, lr.mirror, fs.palette);

    EyeRecipe rr = eyeRecipe(fs.rightShape, true);
    if (rr.s) drawSprite(*rr.s, EYE_R + colOffR, rr.trow + rowOff, rr.mirror, fs.palette);
}

void FaceRenderer::drawMouth(const FaceState& fs) {
    const Sprite* s = nullptr;
    int trow = 15;
    switch (fs.mouth) {
        case MouthShape::NONE:  return;
        case MouthShape::FLAT:  s = &MFLAT;  trow = 15; break;
        case MouthShape::OPEN:  s = &MOPEN;  trow = 14; break;
        case MouthShape::FROWN: s = &MFROWN; trow = 14; break;
    }
    if (s) drawSprite(*s, 15, trow, false, fs.palette);
}

void FaceRenderer::drawDecorations(const FaceState& fs) {
    for (int i = 0; i < fs.numDecos; ++i) {
        const auto& d = fs.decos[i];
        if (!d.sprite) continue;
        drawSprite(*d.sprite, d.ccol, d.trow, d.mirror, fs.palette);
    }
}

void FaceRenderer::applyVignette() {
    // Edge-fade rectangle: outer pixels of FACE rect are blended toward black.
    // Four edge ramps (top/bottom/left/right) of width edgeReach pixels each.
    const int edgeReach = (int)(FACE_W * 0.5f * (1.0f - std::max(0.08f, 1.0f - VIGNETTE_EDGE)));
    if (edgeReach <= 0) return;

    auto blendBlack = [&](int x, int y, uint8_t a) {
        if ((unsigned)x >= CANVAS_W || (unsigned)y >= CANVAS_H) return;
        lv_color_t& px = _buf[y * CANVAS_W + x];
        uint32_t v = lv_color_to_u32(px);
        int r = ((v >> 16) & 0xff) * (255 - a) / 255;
        int g = ((v >> 8) & 0xff) * (255 - a) / 255;
        int b = (v & 0xff) * (255 - a) / 255;
        px = lv_color_make((uint8_t)r, (uint8_t)g, (uint8_t)b);
    };

    const uint8_t maxAlpha = (uint8_t)(VIGNETTE_DIM * 255);

    // Left + right ramps
    for (int i = 0; i < edgeReach; ++i) {
        uint8_t a = (uint8_t)(maxAlpha * (edgeReach - i) / edgeReach);
        for (int y = FACE_Y; y < FACE_Y + FACE_H; ++y) {
            blendBlack(FACE_X + i, y, a);
            blendBlack(FACE_X + FACE_W - 1 - i, y, a);
        }
    }
    // Top + bottom ramps
    int edgeReachV = (int)(FACE_H * 0.5f * (1.0f - std::max(0.08f, 1.0f - VIGNETTE_EDGE)));
    for (int j = 0; j < edgeReachV; ++j) {
        uint8_t a = (uint8_t)(maxAlpha * (edgeReachV - j) / edgeReachV);
        for (int x = FACE_X; x < FACE_X + FACE_W; ++x) {
            blendBlack(x, FACE_Y + j, a);
            blendBlack(x, FACE_Y + FACE_H - 1 - j, a);
        }
    }
}

void FaceRenderer::clipToFaceRect() {
    // Rounded-rect clip: corner cells outside the radius become bezel-black.
    // For each pixel within FACE_R of a corner, compute distance to the
    // corner-circle centre; if outside, paint black.
    const int r = FACE_R;
    auto cornerClip = [&](int cx, int cy, int sx, int sy) {
        for (int y = 0; y < r; ++y) {
            for (int x = 0; x < r; ++x) {
                int dx = x;
                int dy = y;
                if (dx * dx + dy * dy > r * r) {
                    putPixel(cx + sx * x, cy + sy * y, lv_color_black());
                }
            }
        }
    };
    // top-left
    cornerClip(FACE_X + r, FACE_Y + r, -1, -1);
    // top-right
    cornerClip(FACE_X + FACE_W - 1 - r, FACE_Y + r, +1, -1);
    // bottom-left
    cornerClip(FACE_X + r, FACE_Y + FACE_H - 1 - r, -1, +1);
    // bottom-right
    cornerClip(FACE_X + FACE_W - 1 - r, FACE_Y + FACE_H - 1 - r, +1, +1);
}

// ---------------- main render entry ----------------

void FaceRenderer::render(const FaceState& fs) {
    if (!_buf || !_canvas) return;
    if (fs.generation == _lastGen) return;
    _lastGen = fs.generation;

    fillFaceBackground(fs.palette);
    applyRowMaskBackground(fs.palette);
    drawEyes(fs);
    drawMouth(fs);
    drawDecorations(fs);
    applyVignette();
    clipToFaceRect();

    lv_obj_invalidate(_canvas);
}

}  // namespace stackchan::avatar::aino
