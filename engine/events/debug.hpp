#pragma once
#include <string>
#include "engine/events/event.hpp"

namespace tmt {

class OnDrawLines : public EventListenerBase<OnDrawLines, void> {
   public:
    OnDrawLines() : EventListenerBase() {}

    void on_event() final override { on_draw_lines(); }

    virtual void on_draw_lines() const = 0;
    constexpr virtual std::string get_name() const = 0;

    /* [Optional] */
    constexpr virtual bool default_enabled() const { return true; }
};

}  // namespace tmt