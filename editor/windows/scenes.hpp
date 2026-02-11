#pragma once
#include "editor/core/window.hpp"
#include "engine/events/scene.hpp"
#include "editor/events/scene.hpp"

namespace tmt {

class ScenesWindow : public IWindow, OnSceneModified, OnSceneSerialized, OnPostLoadScene {
   public:
    ScenesWindow() = default;
    ~ScenesWindow() = default;

    bool is_scene_dirty() const { return dirty_scene; }

   private:
    bool dirty_scene = false;

    // Inherited via IWindow
    void display() override;

    void on_editor_start() override;
    void on_editor_update(const FrameData& time) override;
    void on_editor_end() override;

    std::string get_title() const override { return "Scenes"; };

    // Inherited via OnSceneModified
    void on_scene_modified() override;

    // Inherited via OnSceneSerialized
    void on_scene_serialized() override;

    // Inherited via OnPostLoadScene
    void on_post_load_scene() override;
};

}  // namespace tmt