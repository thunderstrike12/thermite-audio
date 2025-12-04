#pragma once
#include "event.hpp"
#include "engine/core/application.hpp"

namespace tmt {
class OnEngineInit : public EventListenerBase<OnEngineInit, ApplicationSpecs> {
   public:
    OnEngineInit() : EventListenerBase() {}

    virtual void on_engine_init(const ApplicationSpecs& specs) = 0;

    void on_event(ApplicationSpecs&) final override { /* Empty */ };
    void on_event(const ApplicationSpecs& specs) final override { on_engine_init(specs); }
};

class OnEngineUpdate : public EventListenerBase<OnEngineUpdate, FrameData> {
   public:
    OnEngineUpdate() : EventListenerBase() {}

    virtual void on_engine_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_engine_update(time); }
};

class OnEngineFixedUpdate : public EventListenerBase<OnEngineFixedUpdate, FrameData> {
   public:
    OnEngineFixedUpdate() : EventListenerBase() {}

    virtual void on_engine_fixed_update(const FrameData& time) = 0;
    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_engine_fixed_update(time); }
};

class OnEngineEnd : public EventListenerBase<OnEngineEnd, void> {
   public:
    OnEngineEnd() : EventListenerBase() {}

    virtual void on_engine_end() = 0;

    void on_event() final override { on_engine_end(); }
};

class OnEndFrame : public EventListenerBase<OnEndFrame, void> {
   public:
    OnEndFrame() : EventListenerBase() {}
    virtual void on_end_frame() = 0;
    void on_event() final override { on_end_frame(); }
};

}  // namespace tmt