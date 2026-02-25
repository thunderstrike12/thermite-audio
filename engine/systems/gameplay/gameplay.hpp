#pragma once
#include "engine/core/system.hpp"
#include "engine/systems/gameplay/types.hpp"
#include "engine/events/scene.hpp"

namespace tmt {

class Gameplay : public ISystem, OnPreUnloadScene {
   public:
    constexpr virtual std::string get_name() override { return "Gameplay"; };

    void on_start() override;

    void on_update(const tmt::FrameData& time) override;

    void on_end() override;

    void on_fixed_update(const tmt::FrameData&) override;

    /* Component management */
    void register_component_instance(const ComponentIndex& index, std::weak_ptr<IGameComponent> component);
    const std::vector<std::weak_ptr<IGameComponent>>& get_component_instances(const ComponentIndex& index) const;

   private:
    std::unordered_map<ComponentIndex, std::vector<std::weak_ptr<IGameComponent>>> component_instances;

    void clear_component_instances() { component_instances.clear(); }

    // Inherited via OnPreUnloadScene
    void on_pre_unload_scene() override;
};

}  // namespace tmt