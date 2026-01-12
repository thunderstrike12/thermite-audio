#pragma once
#include <nlohmann/json.hpp>
#include "engine/core/frame_data.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/all.hpp"
#include "engine/core/components/component_collection.hpp"
#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
#include <ImReflect.hpp>
#include "editor/imgui/types/all.hpp"
#include "editor/imgui/components/all.hpp"
#else
class ImSettings;
class ImResponse;
#endif

namespace tmt {
/* Used by engine to call events */
class IGameComponent {
   public:
    IGameComponent(Entity entity) : entity(std::move(entity)) {};
    virtual ~IGameComponent() = default;

    const Entity entity = entt::null;

    /* [ Required ] defined by user */
    virtual std::string_view get_name() const = 0;

    /* Gameplay events */
    /* [ Required ] */
    virtual void start() = 0;
    virtual void update(const FrameData& time) = 0;
    virtual void end() = 0;
    /* [ Optional ] */
    virtual void fixed_update(const FrameData& time) { (void)time; };

    /* [ Auto ] Helpers for engine */
    virtual nlohmann::json serialize() const = 0;
    virtual void deserialize(const nlohmann::json& value) = 0;
    virtual void inspect(ImSettings& settings, ImResponse& response) = 0;
};

/* Compiled during game/exe compilation */
template <class Derived>
class GameComponent : public IGameComponent {
   public:
    using IGameComponent::IGameComponent;
    /* [ Auto ] Implemented by default in-engine so user don't have to */
    nlohmann::json serialize() const override { return Serializer::serialize(static_cast<const Derived&>(*this)); }

    /* [ Auto ] Implemented by default in-engine so user don't have to */
    void deserialize(const nlohmann::json& value) override { Serializer::deserialize(value, static_cast<Derived&>(*this)); }

    static std::string_view name() { return Derived::get_name(); }

    std::string_view get_name() const final override { return Derived::name(); }

    /* [ Optional ] inspect function to add custom imgui code */
    virtual void on_inspect(ImResponse& response) {};

    /* [ Auto ] Implemented by default in-engine so user don't have to */
    void inspect(ImSettings& settings, ImResponse& response) override {
#if defined(THERMITE_EDITOR) && !defined(THERMITE_ENGINE)
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