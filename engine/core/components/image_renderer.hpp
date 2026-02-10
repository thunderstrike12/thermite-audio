#pragma once

#include "engine/core/resources/texture_2d.hpp"

namespace tmt {

struct ImageRenderer {
    ResourceRef<Texture2D> texture;
    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    ImageRenderer() = default;
    ImageRenderer(const ResourceRef<Texture2D> texture);
    ~ImageRenderer() = default;
};

}  // namespace tmt

TMT_COMPONENT(tmt::ImageRenderer, "ImageRenderer", (texture, color));