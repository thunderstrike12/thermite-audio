#pragma once
#include "editor/core/window.hpp"
#include "engine/events/scene.hpp"
#include "editor/events/scene.hpp"

namespace tmt {

class ScenesWindow : public IEditorSystem, OnSceneModified, OnSceneSerialized, OnPostLoadScene {
   public:
    ScenesWindow() = default;
    ~ScenesWindow() = default;

    bool is_scene_dirty() const { return dirty_scene; }

   private:
    bool dirty_scene = false;

    // Inherited via IEditorSystem
    void on_editor_start() override;
    void on_editor_update(const FrameData& time) override;
    void on_editor_end() override;

    // Inherited via OnSceneModified
    void on_scene_modified() override;

    // Inherited via OnSceneSerialized
    void on_scene_serialized() override;

    // Inherited via OnPostLoadScene
    void on_post_load_scene() override;
};

}  // namespace tmt