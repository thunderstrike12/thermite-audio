#pragma once
#include "editor/core/window.hpp"
#include "engine/core/io.hpp"
#include "engine/tools/scene_types.hpp"

#include <typeindex>
namespace tmt {

class BuildPackager : public IWindow {
   public:
    void on_inspect() override;
    constexpr std::string get_title() const override { return ICON_MS_PACKAGE " Build Packager"; }

   private:
    enum class Status { IDLE, BUILDING, SUCCESS, FAILED };
    Status status = Status::IDLE;
    std::string status_message;
    std::filesystem::path last_built_exe;
    SceneIndex selected_scene = NULL_SCENE;
    std::string build_name = "game_build";

    std::filesystem::path get_root_folder() const;
    std::filesystem::path get_build_folder() const;
    std::filesystem::path get_unique_zip_path() const;
    std::filesystem::path find_game_exe() const;

    // use windows powershell to make compression simple
    bool create_package();
    void play_build();
};

}  // namespace tmt
