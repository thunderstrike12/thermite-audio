#pragma once
#include <graphite/imgui.hh>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "engine/events/engine.hpp"
#include "engine/events/sdl.hpp"

#include "editor/core/font_manager.hpp"

namespace tmt {
class ImGuiManager : public internal::OnSdlEvent, public OnEngineUpdate {
   public:
    ImGuiManager() = default;

    void init();
    void new_frame();
    void end_frame();
    void deinit();

    FontManager font_manager;

   private:
    void on_sdl_event(SDL_Event& event) override;

    ImGUI imgui {};

    // Inherited via OnEngineUpdate
    void on_engine_update(const FrameData& time) override;
};
}  // namespace tmt