#pragma once
#include <glm/glm.hpp>

namespace tmt {

/* Represents a renderable view in the scene. (used on the GPU) */
struct RenderView {
    /* World-space to clip-space transformation matrix. */
    glm::mat4 world_to_clip {};
    /* Clip-space to world-space transformation matrix. */
    glm::mat4 clip_to_world {};
    /* Origin of the view in world-space. */
    glm::vec4 origin {};
    /* Resolution of the view in pixels. */
    glm::uvec2 resolution {};
};

}  // namespace tmt
