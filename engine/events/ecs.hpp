#pragma once
#include "engine/events/event.hpp"
#include "engine/core/entity.hpp"

namespace tmt {

class OnDisableEntity : public EventListenerBase<OnDisableEntity, tmt::Entity> {
   public:
    OnDisableEntity() : EventListenerBase() {}
    virtual void on_disable_entity(const tmt::Entity& entity) = 0;

    void on_event(tmt::Entity& entity) final override { on_disable_entity(entity); }
    void on_event(const tmt::Entity& entity) final override { on_disable_entity(entity); }
};

class OnEnableEntity : public EventListenerBase<OnEnableEntity, tmt::Entity> {
   public:
    OnEnableEntity() : EventListenerBase() {}
    virtual void on_enable_entity(const tmt::Entity& entity) = 0;

    void on_event(tmt::Entity& entity) final override { on_enable_entity(entity); }
    void on_event(const tmt::Entity& entity) final override { on_enable_entity(entity); }
};

}  // namespace tmt