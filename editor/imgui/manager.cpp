#include "editor/imgui/manager.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include "engine/events/sdl.hpp"

#include "engine/engine.hpp"

#include "engine/core/window.hpp"
#include "engine/core/io.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "ImGuizmo.h"

namespace tmt {

void ImGuiManager::init() {
    /* Initialize the immediate mode GUI */
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(engine.window.window);
    imgui.set_clear_screen(true);
    tmt::engine.renderer.set_imgui(&imgui);

    /* Initialize font manager */
    font_manager.init();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    const auto ini_location = IO::FileLocation(IO::Location::EDITOR, "save_data/imgui.ini");
    /* Needs static storage duration */
    static const auto ini_path = ini_location.get_absolute_path().string();
    io.IniFilename = ini_path.c_str();
}

void ImGuiManager::new_frame() {
    /* Reload fonts if needed */
    if (font_manager.pending_reload) font_manager.reload_fonts();

    /* Start a new imgui frame */
    imgui.new_frame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    /* main docking window*/
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
}

void ImGuiManager::end_frame() {
    /* End the imgui frame */
    ImGui::Render();
}

void ImGuiManager::deinit() { imgui.deinit().expect("Failed to deinitialize ImGui"); }

void ImGuiManager::on_sdl_event(SDL_Event& event) { ImGui_ImplSDL3_ProcessEvent(&event); }

void ImGuiManager::on_engine_update(const FrameData&) {}

}  // namespace tmt