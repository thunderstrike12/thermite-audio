#pragma once

#include <graphite/resources/handle.hh>

#include "engine/core/resource.hpp"

namespace tmt {

class LUT : public FileResource {
   public:
    LUT(IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    // bool fallback(FallbackReason reason) override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".cube" };

    Texture texture {};
    Image image {};

    float lut_size = 32.0f;

    std::string name = "Unknown LUT";
};

}  // namespace tmt