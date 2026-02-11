#pragma once
#include "engine/events/event.hpp"

namespace tmt {

/* Whenever a property in the scene is modified (entities, components, etc) */
class OnSceneModified : public EventListenerBase<OnSceneModified, void> {
   public:
    OnSceneModified() : EventListenerBase() {}
    virtual void on_scene_modified() = 0;
    void on_event() final override { on_scene_modified(); }
};

}  // namespace tmt