#pragma once
#include "event.hpp"
#include "engine/core/frame_data.hpp"

namespace tmt {

class OnStart : public EventListenerBase<OnStart, void> {
   public:
    OnStart() : EventListenerBase() {}
    virtual void on_start() = 0;
    void on_event() final override { on_start(); }
};

class OnUpdate : public EventListenerBase<OnUpdate, FrameData> {
   public:
    OnUpdate() : EventListenerBase() {}

    virtual void on_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_update(time); }
};

class OnFixedUpdate : public EventListenerBase<OnFixedUpdate, FrameData> {
   public:
    OnFixedUpdate() : EventListenerBase() {}

    virtual void on_fixed_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_fixed_update(time); }
};

class OnEnd : public EventListenerBase<OnEnd, void> {
   public:
    OnEnd() : EventListenerBase() {}
    virtual void on_end() = 0;
    void on_event() final override { on_end(); }
};

}  // namespace tmt