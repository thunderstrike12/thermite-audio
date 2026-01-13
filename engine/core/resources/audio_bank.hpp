#pragma once

#include "engine/core/resource.hpp"
#include "engine/core/audio.hpp"

namespace FMOD {
namespace Studio {
class Bank;
}
}  // namespace FMOD

namespace tmt {

// FMOD .bank audio resource.
class AudioBank : public FileResource {
   public:
    AudioBank(IO::FileLocation file_location) : FileResource(std::move(file_location)) {}

    bool load() override;
    void unload() override;

    inline static const std::set<std::string_view> SUPPORTED_FILE_EXTENSIONS {".bank"};

    // Get the path/name of the bank (returns empty string when not in debug mode and not using the editor).
    [[nodiscard]] std::string get_path() const;
    [[nodiscard]] std::vector<AudioEvent> get_audio_events() const;
    [[nodiscard]] std::vector<VolumeControl> get_volume_controls() const;

   private:
    // TODO: Replace this with a proper method of handling resource dependencies.
    ResourceRef<AudioBank> master_bank_ref {};
    FMOD::Studio::Bank* bank {nullptr};
    FMOD::Studio::Bank* string_bank {nullptr};
};

}  // namespace tmt