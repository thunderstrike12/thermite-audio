#pragma once
#include "event.hpp"

namespace tmt {

struct MouseOverride : public EventBase {
    float x;
    float y;
};

class OnRetrieveMouseState : public EventListenerBase<OnRetrieveMouseState, MouseOverride> {
   public:
    OnRetrieveMouseState() : EventListenerBase() {}
    virtual void on_retrieve_mouse_state(MouseOverride& event) = 0;
    void on_event(MouseOverride& event) final override { on_retrieve_mouse_state(event); }
    void on_event(const MouseOverride& event) final override { (void)event; /* Empty */ }
};

}  // namespace tmt
