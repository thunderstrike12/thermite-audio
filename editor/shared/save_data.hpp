#pragma once
#include <unordered_map>
#include <engine/core/reflection.hpp>
#include "engine/core/io.hpp"

namespace tmt {

struct SnapValues {
    float move = 1.0f;
    float rotation = 15.0f;
    float scale = 0.1f;
};
struct FontSize {
    float text_size { -1.0f };
    float icon_size { -1.0f };
};

struct SaveData {
    struct Config {
        static inline const IO::FileLocation FILE_LOCATION { IO::Location::EDITOR, "save_data/editor_save.json" };
    };

    void save() const {
        const json j = tmt::Serializer::serialize(*this);
        IO::write_text_file(Config::FILE_LOCATION, j.dump(), true);
    }

    void load() {
        const std::string file_content = IO::read_or_create_text_file(Config::FILE_LOCATION);
        if (file_content.empty()) return;
        const json j = json::parse(file_content);
        tmt::Serializer::deserialize(j, *this);
    }

    std::unordered_map<std::string, bool> open_windows;
    std::unordered_map<std::string, bool> enabled_debug_renderers;

    SnapValues snap_values;
    FontSize font_size;
};

}  // namespace tmt
TMT_OBJECT(tmt::FontSize, (text_size, icon_size));

TMT_OBJECT(tmt::SnapValues, (move, rotation, scale));
TMT_OBJECT_EX(tmt::SaveData, (open_windows, enabled_debug_renderers, snap_values), (snap_values));