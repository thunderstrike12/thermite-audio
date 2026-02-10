#pragma once

#include "engine/core/io.hpp"

#include "editor/core/mode.hpp"
#include "engine/core/scene.hpp"

namespace tmt {

class PrefabEditScene : public Scene<PrefabEditScene> {
   public:
    static constexpr std::string_view scene_name() { return "PrefabEditScene"; }
};

class PrefabMode : public IEditorMode {
   public:
    // Inherited via IEditorMode.
    constexpr std::string get_name() override { return "Prefab"; }

    void display_main_menu() override;

    void on_switch_to(const std::any& meta_data = {}) override;
    void on_switch_away() override;

   private:
    IO::FileLocation prefab_location;

    void save_prefab();
};

}  // namespace tmt