#pragma once

#include "engine/core/entity.hpp"

#include <span>

// #include <glm/fwd.hpp>

namespace tmt {

// Modifier flags for bounds manipulation
struct BoundsModifiers {
    bool uniform_scale = false;      // Shift: maintain aspect ratio
    bool scale_from_center = false;  // Alt: keep center fixed (adjusts position)
};

class Gizmo {
   public:
    Gizmo() = default;

    void init();

    bool hovered() const;
    bool manip(float x, float y, float width, float height, const std::span<const Entity>& selected_entities, float snap, const BoundsModifiers& modifiers) const;

    static constexpr uint8_t GIZMO_OP_COUNT { 4 };
    // Translate=7, Rotate=120, Scale=896, Bounds=1024
    static constexpr int OPERATIONS[GIZMO_OP_COUNT] { 7, 120, 896, 1024 };
    static const char* gizmo_op_icons[GIZMO_OP_COUNT];

    int space { 1 };
    uint8_t multiselect_mode { 0 };
    uint8_t operation { 0 };

   private:
    void setup_style();
};

}  // namespace tmt