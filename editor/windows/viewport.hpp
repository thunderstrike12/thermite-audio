#pragma once
#include "editor/core/window.hpp"

namespace tmt {
class Viewport : public IWindow {
   public:
    Viewport() = default;
    ~Viewport() = default;

    // Inherited via IWindow
    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    void before_begin() override;
    void display() override;
    void end_display() override;

    constexpr std::string get_title() const override { return "Viewport"; };
    constexpr bool is_closable() const override { return false; };

   private:
    void gizmo_manip();
    void setup_gizmo_style();

    uint8_t gizmo_space = 1;
    uint8_t gizmo_multiselect_mode = 0;
    uint8_t gizmo_operation = 0;

    static constexpr uint8_t GIZMO_OP_COUNT = 3;

    uint32_t gizmo_operations[GIZMO_OP_COUNT] = {7, 120, 896};

    float width = -1;
    float height = -1;
};
}  // namespace tmt