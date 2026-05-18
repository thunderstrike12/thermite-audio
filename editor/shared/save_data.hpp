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

    void save() const;
    void load();

    std::unordered_map<std::string, bool> open_windows;
    std::unordered_map<std::string, bool> enabled_debug_renderers;

    SnapValues snap_values;
    FontSize font_size;

    std::optional<float> fps_limit;
};

}  // namespace tmt
TMT_OBJECT(tmt::FontSize, (text_size, icon_size));

TMT_OBJECT(tmt::SnapValues, (move, rotation, scale));
TMT_OBJECT_EX(tmt::SaveData, (open_windows, enabled_debug_renderers, snap_values, fps_limit), (snap_values));