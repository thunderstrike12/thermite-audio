#pragma once
#include "engine/core/system.hpp"
#include <glm/glm.hpp>

namespace tmt {

/* forward declare */
struct UIComponent;
struct Transform;

class UI : public ISystem {
   public:
    // Inherited via ISystem
    std::string get_name() override { return "UI"; }

    void on_start() override;
    void on_update(const tmt::FrameData& time) override;
    void set_size(tmt::UIComponent& ui_component, const tmt::Transform& transform, const glm::uvec2& screen_size);
    void on_end() override;
};

}  // namespace tmt
