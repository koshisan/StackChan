/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Aino renderer — owns an LVGL canvas with an off-screen RGB565 buffer the
 * size of the screen (320×240). Each render() call repaints the entire canvas
 * from the FaceState snapshot following the design's draw order:
 *   bezel → bg → row-mask-bg → glow blobs → sprite cells → row-mask-sprite → vignette
 *
 * The whole canvas is then flushed to the LVGL parent once per state-change
 * (~30 fps when animating, idle otherwise).
 */
#pragma once
#include "aino_face_state.h"
#include "aino_geometry.h"
#include <lvgl.h>
#include <cstdint>

namespace stackchan::avatar::aino {

class FaceRenderer {
public:
    FaceRenderer() = default;
    ~FaceRenderer();
    FaceRenderer(const FaceRenderer&)            = delete;
    FaceRenderer& operator=(const FaceRenderer&) = delete;

    /** Attach to LVGL parent; allocates the canvas buffer (~150KB RGB565). */
    void init(lv_obj_t* parent);

    /** Repaint the canvas from `state`. No-op if generation unchanged. */
    void render(const FaceState& state);

    /** Force-redraw next call regardless of generation. */
    void invalidate() { _lastGen = 0xFFFFFFFF; }

    lv_obj_t* getCanvas() const { return _canvas; }

private:
    lv_obj_t* _canvas             = nullptr;
    lv_color_t* _buf              = nullptr;   // points into PSRAM-allocated buffer
    uint32_t _lastGen             = 0xFFFFFFFF;

    // ----- draw primitives operating on _buf -----
    inline void putPixel(int x, int y, lv_color_t c);
    void fillRect(int x, int y, int w, int h, lv_color_t c);
    void fillFaceBackground(const Palette& p);
    void applyRowMaskBackground(const Palette& p);
    void blitGlow(int cx, int cy, const Palette& p, float scale);
    void blitSpriteCells(const Sprite& s, int ccol, int trow, bool mirror,
                         const Palette& p);
    void drawSprite(const Sprite& s, int ccol, int trow, bool mirror,
                    const Palette& p);
    void drawEyes(const FaceState& fs);
    void drawMouth(const FaceState& fs);
    void drawDecorations(const FaceState& fs);
    void applyVignette();
    void clipToFaceRect();   // round-rect clip for the CRT corners
};

}  // namespace stackchan::avatar::aino
