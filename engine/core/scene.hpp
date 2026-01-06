#pragma once
#include <string>
#include <stdexcept>

#include "engine/events/game.hpp"

namespace tmt {

/* CTRP design for the name */
class SceneBase : public IGameEvents {
   public:
    SceneBase() = default;
    virtual ~SceneBase() = default;

    /* [Optional] */
    virtual void on_start() override {};
    virtual void on_update(const tmt::FrameData& time) override {};
    virtual void on_end() override {};
    virtual void on_fixed_update(const tmt::FrameData& time) override {};

    /* [Optional] */
    /* Gets called before deserialization */
    virtual void on_pre_load() {};
    /* Gets called after deserialization */
    virtual void on_post_load() {};

    virtual std::string_view get_name() const = 0;
};

/*CTRP design for the name */
template <typename Derived>
class Scene : public SceneBase {
   public:
    static std::string_view scene_name() { return Derived::get_name(); }

    std::string_view get_name() const final override { return Derived::scene_name(); }
};

}  // namespace tmt