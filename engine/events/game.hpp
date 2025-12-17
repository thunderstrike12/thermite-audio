#pragma once
#include "event.hpp"
#include "engine/core/frame_data.hpp"

namespace tmt {

/*
This is an interface only, these functions do not get called auotmatically
Use OnEvents below to listen to game events
 */
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

class OnGamePause : public EventListenerBase<OnGamePause, void> {
   public:
    OnGamePause() : EventListenerBase() {}
    virtual void on_game_pause() = 0;
    void on_event() final override { on_game_pause(); }
};

class OnGameResume : public EventListenerBase<OnGameResume, void> {
   public:
    OnGameResume() : EventListenerBase() {}
    virtual void on_game_resume() = 0;
    void on_event() final override { on_game_resume(); }
};

class OnGameEnd : public EventListenerBase<OnGameEnd, void> {
   public:
    OnGameEnd() : EventListenerBase() {}
    virtual void on_game_end() = 0;
    void on_event() final override { on_game_end(); }
};

}  // namespace tmt