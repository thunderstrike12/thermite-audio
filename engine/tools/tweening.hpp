#pragma once
#include "engine/tools/serializer.hpp"
#include "engine/tools/component_registry.hpp"

#define BEFRIEND_MIXINS BEFRIEND_VISITABLE();
#include <../type_tween/type_tween.hpp>

#include "engine/tools/profiler.hpp"

namespace tmt {

// clang-format off
template <typename Derived, typename T>
using TweenMixinsSerialize = ComponentRegistry<
    /*Order in which mixins are displayed */
    Tweening::Detail::FromToMixin<Derived, T>,
    Tweening::Detail::EaseMixin<Derived>,
    Tweening::Detail::TimeMixin<Derived>,
    Tweening::Detail::LoopMixin<Derived>,
    Tweening::Detail::RepeatMixin<Derived>
    //Tweening::Detail::CallbackMixin<Derived>
    //Tweening::Detail::TypedCallbackMixin<Derived, T>
    //Tweening::Detail::ControllableMixin<Derived>
    >;
// clang-format on

// clang-format off
template <typename Derived, typename T>
using TweenMixinsInspect = ComponentRegistry<
    /*Order in which mixins are displayed */
    Tweening::Detail::FromToMixin<Derived, T>,
    Tweening::Detail::EaseMixin<Derived>,
    Tweening::Detail::RepeatMixin<Derived>,
    Tweening::Detail::LoopMixin<Derived>,
    Tweening::Detail::TimeMixin<Derived>,
    //Tweening::Detail::CallbackMixin<Derived>,
    //Tweening::Detail::TypedCallbackMixin<Derived, T>,
    Tweening::Detail::ControllableMixin<Derived>
    >;
// clang-format on

class Tweener : public ISystem {
    constexpr virtual std::string get_name() override { return "Tweener"; }

    virtual void on_start() override {};
    virtual void on_update(const tmt::FrameData& time) override {
        TMT_ZONE_SCOPED_N("Tweening");
        Tweening::update(time.delta_time);
    };
    virtual void on_end() override { Tweening::clear(); };
};

}  // namespace tmt

// VISITABLE_TEMPLATE_STRUCT_IN_CONTEXT(ImReflect::Detail::ImContext, (typename D), Tweening::Detail::EaseMixin, (D), ease_value, reverse_ease_value);

TMT_OBJECT_TEMPLATE((typename Derived, typename T), Tweening::Detail::FromToMixin, (Derived, T), start_value, end_value);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::EaseMixin, (Derived), ease_value, reverse_ease_value);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::TimeMixin, (Derived), duration_value, start_delay_value, reverse_delay_value, repeat_delay_value, end_delay_value);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::LoopMixin, (Derived), loop_mode);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::RepeatMixin, (Derived), repeat_count);
TMT_OBJECT_TEMPLATE(
    (typename Derived), Tweening::Detail::CallbackMixin, (Derived), on_pre_start_cb, on_start_cb, on_cycle_begin_cb, on_forward_end_cb, on_reverse_begin_cb, on_reverse_end_cb, on_cycle_end_cb,
    on_repeat_cb, on_complete_cb
);
TMT_OBJECT_TEMPLATE((typename Derived, typename T), Tweening::Detail::TypedCallbackMixin, (Derived, T), on_update_cb);
TMT_OBJECT_TEMPLATE((typename Derived), Tweening::Detail::ControllableMixin, (Derived), ctrl_stop, ctrl_start, ctrl_restart, ctrl_finish, ctrl_is_paused);
