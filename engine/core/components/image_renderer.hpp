#pragma once

#include "engine/core/resources/texture_2d.hpp"
#include "engine/tools/types/color.hpp"

namespace tmt {

struct ImageRenderer {
    ResourceRef<Texture2D> texture;
    RGBA color = RGBA(glm::vec4(1, 1, 1, 1));

    ImageRenderer() = default;
    ImageRenderer(const ResourceRef<Texture2D> texture);
    ~ImageRenderer() = default;
};

}  // namespace tmt

TMT_COMPONENT(tmt::ImageRenderer, "ImageRenderer", (texture, color));