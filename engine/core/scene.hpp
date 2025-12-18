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