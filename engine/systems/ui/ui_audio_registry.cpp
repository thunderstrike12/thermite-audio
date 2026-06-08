#include "ui_audio_registry.hpp"
#include "engine/tools/serializer/all.hpp"
#include "engine/tools/serializer.hpp"

void tmt::UiAudioRegistry::load() {
    const std::string file_data = tmt::IO::read_text_file({ tmt::IO::Location::PROJECT, "audio/menu_sounds.json" });

    if (file_data.empty()) {
        tmt::Log::warn("No menu sounds found.");
        return;
    }

    tmt::json j {};

    try {
        j = tmt::json::parse(file_data);
    } catch (const std::exception& e) {
        tmt::Log::error("Failed to parse menu_sounds.json: {}", e.what());
        return;
    }

    tmt::Serializer::deserialize(j, *this);
}

void tmt::UiAudioRegistry::save() const {
    tmt::json j = tmt::Serializer::serialize(*this);

    tmt::IO::write_text_file({ tmt::IO::Location::PROJECT, "audio/menu_sounds.json" }, j.dump(4));
}
