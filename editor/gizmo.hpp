#pragma once

#include "engine/core/entity.hpp"

#include <span>

// #include <glm/fwd.hpp>

namespace tmt {

class Gizmo {
   public:
    Gizmo() = default;

    void init();

    bool manip(float x, float y, float width, float height, const std::span<const Entity>& selected_entities) const;

    static constexpr uint8_t GIZMO_OP_COUNT {3};
    static constexpr int OPERATIONS[GIZMO_OP_COUNT] {7, 120, 896};
    static const char* gizmo_op_icons[GIZMO_OP_COUNT];

    int space {1};
    uint8_t multiselect_mode {0};
    uint8_t operation {0};

   private:
    void setup_style();
};

}  // namespace tmt