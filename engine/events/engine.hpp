#pragma once
#include "event.hpp"
#include "core/application.hpp"

namespace tmt {
class OnEngineInit : public EventListenerBase<OnEngineInit, ApplicationSpecs> {
   public:
    OnEngineInit() : EventListenerBase() {}

    virtual void on_engine_init(const ApplicationSpecs& specs) = 0;

    void on_event(ApplicationSpecs&) final override { /* Empty */ };
    void on_event(const ApplicationSpecs& specs) final override { on_engine_init(specs); }
};

class OnEngineEnd : public EventListenerBase<OnEngineEnd, void> {
   public:
    OnEngineEnd() : EventListenerBase() {}

    virtual void on_engine_end() = 0;

    void on_event() final override { on_engine_end(); }
};

}  // namespace tmt