#pragma once
/*
For INTERNAL use only
should not be used by the game
*/

#include <SDL3/SDL_events.h>

#include "event.hpp"

namespace tmt::internal {

class OnSdlEvent : public EventListenerBase<OnSdlEvent, SDL_Event> {
   public:
    OnSdlEvent() : EventListenerBase() {}

    virtual void on_sdl_event(SDL_Event& event) = 0;

    void on_event(SDL_Event& event) final override { on_sdl_event(event); };
    void on_event(const SDL_Event&) final override { /* Empty */ }
};

}  // namespace tmt::internal