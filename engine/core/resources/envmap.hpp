#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/resource.hpp"

namespace tmt {

std::vector<char> import_envmap(const IO::FileLocation& file_location);

/* Environment Map Resource - used as skydome when rendering */
class Envmap : public FileResource {
   public:
    Envmap(IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    bool load_hdr();

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".env" };

    Texture full_texture {};
    Image full_image {};
    Texture filtered_texture {};
    Image filtered_image {};

    uint32_t width {};
    uint32_t height {};

    std::string name = "Unnamed Environment Map";
};

}  // namespace tmt
