/*
 * SPDX-FileCopyrightText: 2026 koshisan
 * SPDX-License-Identifier: MIT
 *
 * Aino face composer — v2 using LVGL Container hierarchy instead of a
 * direct-buffer canvas (which never showed our writes in LVGL 9.4).
 *
 * Layout:
 *   panel (320×240, parent, black)
 *   ├── face Container (240×180 at FACE_X/Y, bg = palette.bg, radius = FACE_R)
 *   ├── row-mask strips (240×8 each at odd ART-row offsets, bg = palette.bgDim)
 *   └── pool of sprite-cell Containers (8×8 each, bg = palette.fg / fgDim)
 *
 * On every state change, the row-mask strips re-tint and the sprite-cell pool
 * is repositioned/re-tinted. Pool size is bounded by the largest possible
 * lit-cell count across all 10 emotions.
 *
 * No PSRAM allocations; LVGL handles all rendering and refresh.
 */
#pragma once
#include "aino_face_state.h"
#include "aino_geometry.h"
#include <lvgl.h>
#include <smooth_lvgl.hpp>
#include <memory>
#include <vector>
#include <cstdint>

namespace stackchan::avatar::aino {

class FaceComposer {
public:
    FaceComposer() = default;
    ~FaceComposer();
    FaceComposer(const FaceComposer&)            = delete;
    FaceComposer& operator=(const FaceComposer&) = delete;

    /** Build the static scaffolding (face container + row-mask strips) under `parent`. */
    void init(lv_obj_t* parent);

    /** Re-tint background + redraw sprite cells from FaceState. */
    void render(const FaceState& state);

    /** Force re-render on next render() call. */
    void invalidate() { _lastGen = 0xFFFFFFFF; }

    lv_obj_t* getFaceContainer() const { return _face; }

private:
    lv_obj_t* _parent          = nullptr;
    lv_obj_t* _face            = nullptr;        // 240×180 lit area
    std::vector<lv_obj_t*> _maskStrips;          // odd-row 240×8 strips
    std::vector<lv_obj_t*> _cells;               // dynamic pool of 8×8 sprite cells
    uint32_t _lastGen          = 0xFFFFFFFF;

    void ensureCellCount(size_t needed);
    void drawSpriteCells(const Sprite& s, int ccol, int trow, bool mirror,
                         const Palette& p, size_t& cellIdx);
    void hideRemainingCells(size_t fromIdx);
    void retintBackground(const Palette& p);
};

}  // namespace stackchan::avatar::aino
