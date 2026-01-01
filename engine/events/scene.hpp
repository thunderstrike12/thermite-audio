#pragma once
#include "event.hpp"
#include "engine/tools/scene_types.hpp"

namespace tmt {

class OnPreUnloadScene : public EventListenerBase<OnPreUnloadScene, void> {
   public:
    OnPreUnloadScene() : EventListenerBase() {}
    virtual void on_pre_unload_scene() = 0;
    void on_event() final override { on_pre_unload_scene(); }
};

class PreLoadSceneEvent : public EventBase {
   public:
    PreLoadSceneEvent(const SceneIndex& scene_index) : scene_index(scene_index) {}
    ~PreLoadSceneEvent() = default;

    const SceneIndex scene_index;

    /* To be loaded json, can be overwritten */
    nlohmann::ordered_json scene_json;
};

class OnPreLoadScene : public EventListenerBase<OnPreLoadScene, PreLoadSceneEvent> {
   public:
    OnPreLoadScene() : EventListenerBase() {}
    virtual void on_pre_load_scene(PreLoadSceneEvent& event) = 0;
    void on_event(PreLoadSceneEvent& event) final override { on_pre_load_scene(event); }
    void on_event(const PreLoadSceneEvent&) final override { throw std::runtime_error("OnPreLoadScene::on_event: const reference not supported"); }
};

class OnPostLoadScene : public EventListenerBase<OnPostLoadScene, void> {
   public:
    OnPostLoadScene() : EventListenerBase() {}
    virtual void on_post_load_scene() = 0;
    void on_event() final override { on_post_load_scene(); }
};

class OnSceneStart : public EventListenerBase<OnSceneStart, void> {
   public:
    OnSceneStart() : EventListenerBase() {}
    virtual void on_scene_start() = 0;
    void on_event() final override { on_scene_start(); }
};

class OnSceneEnd : public EventListenerBase<OnSceneEnd, void> {
   public:
    OnSceneEnd() : EventListenerBase() {}
    virtual void on_scene_end() = 0;
    void on_event() final override { on_scene_end(); }
};

class OnSceneSerialized : public EventListenerBase<OnSceneSerialized, void> {
   public:
    OnSceneSerialized() : EventListenerBase() {}
    virtual void on_scene_serialized() = 0;
    void on_event() final override { on_scene_serialized(); }
};

}  // namespace tmt