#pragma once
#include "engine/core/system.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

class Stencil;

class Destruction : public ISystem {
   public:
    Destruction() = default;

    // Inherited via ISystem
    std::string get_name() override { return "Destruction System"; }
    void on_start() override;
    void on_update(const FrameData& time) override;
    void on_fixed_update(const FrameData& time) override;
    void on_end() override;

    void destroy_voxels(Entity entity, const Stencil* stencil, glm::ivec3 offset = glm::ivec3(0));

   private:
    std::vector<Entity> find_seperations(Entity entity, const Stencil* stencil, glm::ivec3 offset);
};

}  // namespace tmt