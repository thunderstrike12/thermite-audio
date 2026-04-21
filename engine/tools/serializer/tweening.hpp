#pragma once

#include "engine/tools/tweening.hpp"
#include "engine/tools/serializer.hpp"

// JSON_REFLECT(Tweening::Detail::EaseMixin, ease_value, reverse_ease, has_reverse_ease);

template <typename T, typename... Args>
inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const Tweening::TypedTween<T, Args...>& tween) {
    using TweenTypeHandle = Tweening::TypedTween<T, Args...>;
    using TweenType = typename TweenTypeHandle::TweenType;

    JsonReflect::json result;

    constexpr bool IS_REFLECTED = JsonReflect::Detail::is_visitable_v<TweenType, JsonReflect::serialize_lib_t>;
    if constexpr (IS_REFLECTED) {
        // ImReflect::Input("", *value.get_ptr(), settings, response);
        result = JsonReflect::to_json(tween.get_ptr());
    }

    tmt::TweenMixinsSerialize<TweenType, T>::for_each([&](auto tag) {
        using MixinType = typename decltype(tag)::type;
        if constexpr (std::derived_from<TweenType, MixinType>) {
            JsonReflect::json mixin_json = JsonReflect::to_json(static_cast<const MixinType&>(*tween));
            /* Merge mixin_json into result */
            result.update(mixin_json);
        }
    });

    return result;
}

template <typename T, typename... Args>
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, Tweening::TypedTween<T, Args...>& tween) {
    using TweenTypeHandle = Tweening::TypedTween<T, Args...>;
    using TweenType = typename TweenTypeHandle::TweenType;

    constexpr bool IS_REFLECTED = JsonReflect::Detail::is_visitable_v<TweenType, JsonReflect::serialize_lib_t>;
    if constexpr (IS_REFLECTED) {
        // ImReflect::Input("", *value.get_ptr(), settings, response);
        JsonReflect::from_json(j, *tween.get_ptr());
    }

    tmt::TweenMixinsSerialize<TweenType, T>::for_each([&](auto tag) {
        using MixinType = typename decltype(tag)::type;
        if constexpr (std::derived_from<TweenType, MixinType>) {
            JsonReflect::from_json(j, static_cast<MixinType&>(*tween));
        }
    });
}