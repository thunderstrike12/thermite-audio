#include "editor/imgui/manager.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include "engine/events/sdl.hpp"

#include "engine/engine.hpp"

#include "engine/core/window.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

void ImGuiManager::init() {
    /* Initialize the immediate mode GUI */
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(engine.window.window);
    imgui.set_clear_screen(true);
    tmt::engine.renderer.set_imgui(&imgui, IMGUI_FUNCTIONS);
}

void ImGuiManager::new_frame() {
    /* Start a new imgui frame */
    imgui.new_frame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::end_frame() {
    /* End the imgui frame */
    ImGui::Render();
}

void ImGuiManager::deinit() { imgui.deinit().expect("Failed to deinitialize ImGui"); }

void ImGuiManager::on_sdl_event(SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); }

void ImGuiManager::on_engine_update(const FrameData&) {}

}  // namespace tmt