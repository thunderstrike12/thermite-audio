#pragma once
#include <graphite/imgui.hh>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "engine/events/sdl.hpp"

namespace tmt {
class ImGuiManager : public internal::OnSdlEvent {
   public:
    ImGuiManager() = default;

    void init();
    void new_frame();
    void end_frame();
    void deinit();

   private:
    void on_sdl_event(SDL_Event& event) override;

    ImGUI imgui {};
};
}  // namespace tmt