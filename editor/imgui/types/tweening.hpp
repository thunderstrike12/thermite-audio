#pragma once
#include "engine/tools/tweening.hpp"
#include "engine/tools/tweening/all.hpp"
#include <ImReflect.hpp>

template <typename T, typename... Args>
inline void tag_invoke(ImReflect::ImInput_t, const char* label, Tweening::TypedTween<T, Args...>& value, ImSettings& settings, ImResponse& response) {
    using TweenTypeHandle = Tweening::TypedTween<T, Args...>;
    using TweenType = typename TweenTypeHandle::TweenType;

    ImGui::SeparatorText(label);
    ImReflect::Detail::scope_indent indent {};

    if (!value.is_valid()) {
        ImGui::Text("Invalid Tween");
        return;
    }

    constexpr bool CAN_BE_INSPECTED = ImReflect::Detail::has_imreflect_input_v<TweenType>;
    if constexpr (CAN_BE_INSPECTED) {
        ImReflect::Input("", *value.get_ptr(), settings, response);
    }

    tmt::TweenMixinsInspect<TweenType, T>::for_each([&](auto tag) {
        using MixinType = typename decltype(tag)::type;
        if constexpr (std::derived_from<TweenType, MixinType>) {
            ImReflect::Input("", static_cast<MixinType&>(*value), settings, response);
        }
    });
}