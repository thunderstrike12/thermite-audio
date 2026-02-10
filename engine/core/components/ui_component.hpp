#pragma once

#include "engine/core/entity.hpp"

#include "engine/core/reflection.hpp"

namespace tmt {

enum class Anchor { TOP_LEFT, TOP_CENTER, TOP_RIGHT, MIDDLE_LEFT, MIDDLE_CENTER, MIDDLE_RIGHT, BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT };

enum class AspectMode {
    FIT,     // Maintain aspect ratio, ensuring the entire content is visible
    FILL,    // Maintain aspect ratio but fill the container completely (cropping may occur)
    STRETCH  // Ignore aspect ratio
};

class AnchorHelper {
   public:
    static glm::vec2 calculate_anchor_offset_in_rect(const glm::vec2 size, const Anchor anchor);
    static glm::vec2 calculate_anchor_offset(Entity entity);
    static bool is_inside(Entity entity, const glm::vec2& viewport_point);
};

struct UIComponent {
   public:
    glm::vec2 size = glm::vec2(50.0f);
    glm::vec2 pivot = glm::vec2(0.5f, 0.5f); /* center */
    bool block_raycasts = false;
    bool scale_with_parent_x = false;
    bool scale_with_parent_y = false;
    AspectMode aspect_mode = AspectMode::STRETCH;
    glm::vec2 aspect_ratio = glm::vec2(1.0f, 1.0f); /* 1:1 */
    Anchor anchor = Anchor::MIDDLE_CENTER;
};
}  // namespace tmt

TMT_COMPONENT(tmt::UIComponent, "UIComponent", (size, pivot, block_raycasts, scale_with_parent_x, scale_with_parent_y, aspect_mode, aspect_ratio, anchor));