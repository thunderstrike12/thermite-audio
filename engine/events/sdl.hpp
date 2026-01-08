#pragma once
/*
For INTERNAL use only
should not be used by the game
*/

#include <SDL3/SDL_events.h>

#include "event.hpp"

namespace tmt::internal {

struct SdlEvent {
    SdlEvent(SDL_Event event) : event(event) {}

    SDL_Event event;
    bool imgui_capture_mouse = false;
    bool imgui_capture_keyboard = false;
};

class OnSdlEvent : public EventListenerBase<OnSdlEvent, SdlEvent> {
   public:
    OnSdlEvent() : EventListenerBase() {}

    virtual void on_sdl_event(SdlEvent& event) = 0;

    void on_event(SdlEvent& event) final override { on_sdl_event(event); };
    void on_event(const SdlEvent&) final override { /* Empty */ }
};

}  // namespace tmt::internal