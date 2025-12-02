#pragma once
#include "event.hpp"
#include "engine/core/frame_data.hpp"

namespace tmt {

class IGameEvents {
   public:
    virtual ~IGameEvents() = default;

    /* Events */
    /* [Required] */
    virtual void on_start() = 0;
    virtual void on_update(const tmt::FrameData& time) = 0;
    virtual void on_end() = 0;
    /* [Optional] */
    virtual void on_fixed_update(const tmt::FrameData&) {};
};

class OnGameStart : public EventListenerBase<OnGameStart, void> {
   public:
    OnGameStart() : EventListenerBase() {}
    virtual void on_game_start() = 0;
    void on_event() final override { on_game_start(); }
};

class OnGameUpdate : public EventListenerBase<OnGameUpdate, FrameData> {
   public:
    OnGameUpdate() : EventListenerBase() {}

    virtual void on_game_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_game_update(time); }
};

class OnGameFixedUpdate : public EventListenerBase<OnGameFixedUpdate, FrameData> {
   public:
    OnGameFixedUpdate() : EventListenerBase() {}

    virtual void on_game_fixed_update(const FrameData& time) = 0;

    void on_event(FrameData&) final override { /* Empty */ };
    void on_event(const FrameData& time) final override { on_game_fixed_update(time); }
};

class OnGameEnd : public EventListenerBase<OnGameEnd, void> {
   public:
    OnGameEnd() : EventListenerBase() {}
    virtual void on_game_end() = 0;
    void on_event() final override { on_game_end(); }
};

}  // namespace tmt