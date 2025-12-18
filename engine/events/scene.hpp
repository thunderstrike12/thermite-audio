#pragma once
#include "event.hpp"

namespace tmt {

class OnPreUnloadScene : public EventListenerBase<OnPreUnloadScene, void> {
   public:
    OnPreUnloadScene() : EventListenerBase() {}
    virtual void on_pre_unload_scene() = 0;
    void on_event() final override { on_pre_unload_scene(); }
};

class OnPreLoadScene : public EventListenerBase<OnPreLoadScene, void> {
   public:
    OnPreLoadScene() : EventListenerBase() {}
    virtual void on_pre_load_scene() = 0;
    void on_event() final override { on_pre_load_scene(); }
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

}  // namespace tmt