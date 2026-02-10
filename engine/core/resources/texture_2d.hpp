#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/resource.hpp"

namespace tmt {

/* 2D Texture Resource - used for UI and Particles */
class Texture2D : public FileResource {
   public:
    Texture2D(IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS {".png", ".jpg"};

    Texture texture {};
    Image image {};

    uint32_t width {};
    uint32_t height {};
    uint32_t dummy {};

    std::string name = "Unknown Texture";
};

}  // namespace tmt

TMT_OBJECT(tmt::Texture2D, (dummy));