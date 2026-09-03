#pragma once
#include "engine/core/resource.hpp"
#include "engine/core/resources.hpp"
#include <vector>
#include <set>
#include <string_view>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace FMOD {

class Sound;

}

namespace tmt {

struct AudioData : public FileResource {
   public:
    AudioData(const IO::FileLocation& directory);

    bool load() override;
    void unload() override;
    bool reload() override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS { ".wav", ".mp3" };

    std::vector<float> samples;  // mono, normalized [-1, 1]
    int sample_rate = 0;
    int channels = 0;
};

}  // namespace tmt