#pragma once

#include "engine/core/resources/texture_2d.hpp"
#include "engine/tools/types/color.hpp"

namespace tmt {

struct ImageRenderer {
    ResourceRef<Texture2D> texture;
    RGBA color = RGBA(glm::vec4(1, 1, 1, 1));

    bool continuous_anim = false;
    uint32_t loop_count = 10u;  /* plays this many times when continuous_anim == false */
    float anim_speed = 1.0f;    /* frames per second */

    uint32_t current_frame = 0; /* frame index in the flipbook */
    float accumulated_time = 0.0f;
    uint32_t completed_loops = 0u;
    bool finished = false;

    ImageRenderer() = default;
    ImageRenderer(const ResourceRef<Texture2D> texture);
    ~ImageRenderer() = default;
};

}  // namespace tmt

TMT_COMPONENT_EX(tmt::ImageRenderer, "ImageRenderer", (texture, color, continuous_anim, loop_count, anim_speed), (texture, color));