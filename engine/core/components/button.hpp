#pragma once
#include "engine/tools/types/color.hpp"
#include "engine/core/reflection.hpp"
#include "engine/core/entity.hpp"
#include "engine/tools/types/direction.hpp"

namespace tmt {

enum class ButtonState : uint8_t {
    IDLE = 0,
    ON_SELECT = 1,
    SELECTED = 2,
    ON_DESELECT = 3,
    DISABLED = 4,
    ON_CLICK = 5,
    ON_HOLD = 6,
    ON_RELEASE = 7,
};
// enum class Direction { NorthWest, North, NorthEast, East, SouthEast, South, SouthWest, West };

template <typename T, typename... Args>
concept MatchingMemberFn = requires(T* obj, void (T::*fn)(Args...)) {
    { (obj->*fn)(std::declval<Args>()...) } -> std::same_as<void>;
};

template <typename F, typename... Args>
concept MatchingCallable = std::invocable<F, Args...> && std::same_as<std::invoke_result_t<F, Args...>, void>;

template <typename... Args>
struct ButtonCallback {
    std::vector<std::function<void(Args...)>> callbacks;

    ButtonCallback() = default;
    ButtonCallback(std::function<void(Args...)> callback) { callbacks.push_back(callback); }

    void operator()(Args... args) {
        for (auto& callback : callbacks) {
            callback(args...);
        }
    };

    void operator+=(std::function<void(Args...)> callback) { callbacks.push_back(callback); }
    ButtonCallback& operator=(std::function<void(Args...)> callback) {
        callbacks.clear();
        callbacks.push_back(callback);
        return *this;
    }

    template <typename T>
    requires MatchingMemberFn<T, Args...>
    void add(T* obj, void (T::*fn)(Args...)) {
        callbacks.push_back([obj, fn](Args... args) { (obj->*fn)(args...); });
    }

    template <typename F>
    requires MatchingCallable<F, Args...>
    void add(F&& fn) {
        callbacks.push_back(std::forward<F>(fn));
    }

    void clear() { callbacks.clear(); }
};

struct Button {
   public:
    struct Context {
        const Entity entity;
        const ButtonState state;
        const bool disabled;
    };

    ButtonState state = ButtonState::IDLE;

    bool disabled = false;

    ButtonCallback<Button::Context> on_select = {};
    ButtonCallback<Button::Context> on_selected = {};
    ButtonCallback<Button::Context> on_deselect = {};
    ButtonCallback<Button::Context> on_click = {};
    ButtonCallback<Button::Context, float> on_hold = {};
    ButtonCallback<Button::Context> on_release = {};

    /* Color for each state */
    std::array<RGBA, 8> colors = {
        RGBA(glm::vec4(1.0f)),
        RGBA(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)),
        RGBA(glm::vec4(0.6f, 0.6f, 0.6f, 1.0f)),
        RGBA(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)),
        RGBA(glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)),
        RGBA(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)),
        RGBA(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)),
        RGBA(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)),
    };

    /* Navigation: which button to move to for each direction (indexed by Direction enum: NORTH, NORTH_EAST, EAST, ...) */
    std::array<Entity, 8> flow_direction = { entt::null, entt::null, entt::null, entt::null, entt::null, entt::null, entt::null, entt::null };
};

}  // namespace tmt

TMT_COMPONENT(tmt::Button, "Button", (disabled, colors, flow_direction));