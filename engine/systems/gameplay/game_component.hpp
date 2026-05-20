#pragma once
#include <nlohmann/json.hpp>
#include "engine/core/frame_data.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/all.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
    #include <ImReflect.hpp>
    #include "editor/imgui/types/all.hpp"
    #include "editor/imgui/components/all.hpp"
#else
// class ImSettings;
// class ImResponse;
#endif

/* Forward declare */
namespace tmt {

struct SerializeState;
struct DeserializeState;

}  // namespace tmt

namespace tmt {

class SerializationContext {
   private:
    void* state_ptr = nullptr;
    enum class StateType { NONE, SERIALIZE, DESERIALIZE } type = StateType::NONE;

   public:
    SerializationContext() = default;

    // Type-safe setters
    void set_serialize_state(SerializeState* ptr) {
        state_ptr = ptr;
        type = StateType::SERIALIZE;
    }

    void set_deserialize_state(DeserializeState* ptr) {
        state_ptr = ptr;
        type = StateType::DESERIALIZE;
    }

    // Type-safe getters - work with forward declarations
    SerializeState* get_serialize_state() { return type == StateType::SERIALIZE ? static_cast<SerializeState*>(state_ptr) : nullptr; }

    DeserializeState* get_deserialize_state() { return type == StateType::DESERIALIZE ? static_cast<DeserializeState*>(state_ptr) : nullptr; }

    const SerializeState* get_serialize_state() const { return type == StateType::SERIALIZE ? static_cast<const SerializeState*>(state_ptr) : nullptr; }

    const DeserializeState* get_deserialize_state() const { return type == StateType::DESERIALIZE ? static_cast<const DeserializeState*>(state_ptr) : nullptr; }
};

/* Used by engine to call events */
class IGameComponent {
   public:
    IGameComponent(Entity entity) : entity(std::move(entity)) {};
    virtual ~IGameComponent() = default;

    const Entity entity = entt::null;
    bool started = false;

    /* [ Required ] defined by user */
    virtual std::string_view get_name() const = 0;

    /* Gameplay events */
    /* [ Required ] */
    virtual void start() = 0;
    virtual void update(const FrameData& time) = 0;
    virtual void end() = 0;
    /* [ Optional ] */
    virtual void fixed_update(const FrameData& time) { (void)time; };
    virtual void draw_debug_lines() const {};
    /* when switching from disabled to enabled */
    virtual void on_entity_enabled() {};
    /* when switching from enabled to disabled */
    virtual void on_entity_disabled() {};

    /* [ Auto ] Helpers for engine */
    virtual nlohmann::json serialize(SerializationContext& ctx) const = 0;
    virtual void deserialize(const nlohmann::json& value, SerializationContext& ctx) = 0;
    virtual void inspect(ImSettings& settings, ImResponse& response) = 0;
};

/* Compiled during game/exe compilation */
template <class Derived>
class GameComponent : public IGameComponent {
   public:
    using IGameComponent::IGameComponent;
    /* [ Auto ] Implemented by default in-engine so user don't have to */
    nlohmann::json serialize(SerializationContext& ctx) const override {
        constexpr bool VISITABLE = visit_struct::traits::is_visitable<Derived, JsonReflect::serialize_lib_t>::value;
        if constexpr (VISITABLE == false) return tmt::json::object();

        if (auto* state = ctx.get_serialize_state()) {
            return Serializer::serialize(static_cast<const Derived&>(*this), *state);
        }
        return Serializer::serialize(static_cast<const Derived&>(*this));
    }

    void deserialize(const nlohmann::json& value, SerializationContext& ctx) override {
        constexpr bool VISITABLE = visit_struct::traits::is_visitable<Derived, JsonReflect::deserialize_lib_t>::value;
        if constexpr (VISITABLE == false) return;

        if (auto* state = ctx.get_deserialize_state()) {
            Serializer::deserialize(value, static_cast<Derived&>(*this), *state);
        } else {
            Serializer::deserialize(value, static_cast<Derived&>(*this));
        }
    }

    static std::string_view name() { return Derived::get_name(); }

    std::string_view get_name() const final override { return Derived::name(); }

    /* [ Optional ] inspect function to add custom imgui code */
    virtual void on_inspect(ImResponse& response) {};

    /* [ Auto ] Implemented by default in-engine so user don't have to */
    void inspect(ImSettings& settings, ImResponse& response) override {
#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
        constexpr bool VISITABLE = visit_struct::traits::is_visitable<Derived, ImReflect::Detail::ImContext>::value;
        if constexpr (VISITABLE == false) return;

        ImReflect::Input("", static_cast<Derived&>(*this), settings, response);
        on_inspect(response);
#else
        (void)settings;
        (void)response;
        throw std::runtime_error("ImReflect is only available in Thermite Editor builds.");
#endif
    }
};

}  // namespace tmt

/* Hide behind details namespace, to not clutter the tmt namespace */
namespace tmt::details {

template <typename T>
requires std::is_base_of_v<IGameComponent, T>
struct AutoRegister {
    AutoRegister() { GameComponentRegistry::instance().register_component<T>(); }
};

}  // namespace tmt::details

#define TMT_CONCAT_IMPL(a, b) a##b
#define TMT_CONCAT(a, b) TMT_CONCAT_IMPL(a, b)

#define TMT_GAME_COMPONENT_IMPL(Type)                                                                                   \
    namespace {                                                                                                         \
                                                                                                                        \
    static_assert(std::is_base_of_v<tmt::IGameComponent, Type>, "AutoRegister<Type>: Type must derive from IGameComponent"); \
    [[maybe_unused]] tmt::details::AutoRegister<Type> TMT_CONCAT(_auto_reg_, __COUNTER__) {};                                       \
                                                                                                                        \
    }

#define TMT_GAME_COMPONENT(Type, Fields) \
    TMT_GAME_COMPONENT_IMPL(Type)        \
    TMT_OBJECT(Type, Fields)

#define TMT_GAME_COMPONENT_EX(Type, JsonFields, ImguiFields) \
    TMT_GAME_COMPONENT_IMPL(Type)                            \
    TMT_OBJECT_EX(Type, JsonFields, ImguiFields)

#define TMT_GAME_COMPONENT_EMPTY(Type) \
    TMT_GAME_COMPONENT_IMPL(Type)      \
    TMT_OBJECT_EMPTY(Type)
