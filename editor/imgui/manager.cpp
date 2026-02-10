#include "editor/imgui/manager.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <graphite/imgui.hh>

#include "engine/events/sdl.hpp"
#include "engine/engine.hpp"

#include "engine/core/window.hpp"
#include "engine/core/io.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "ImGuizmo.h"
#include "implot.h"
#include "editor/editor.hpp"
#include "editor/windows/viewport.hpp"

namespace tmt {

void ImGuiManager::init() {
    imgui = new ImGUI;

    /* Initialize the immediate mode GUI */
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(engine.window.window);
    imgui->set_clear_screen(true);
    tmt::engine.renderer.set_imgui(imgui);

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
    imgui->new_frame();
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

void ImGuiManager::deinit() {
    ImPlot::DestroyContext();
    imgui->deinit().expect("Failed to deinitialize ImGui");

    delete imgui;
}

void ImGuiManager::on_sdl_event(internal::SdlEvent& event) {
    ImGui_ImplSDL3_ProcessEvent(&event.event);
    const auto* viewport = editor.windows[editor.editor_mode].try_get<Viewport>();

    if (viewport == nullptr) {
        return;
    }

    if (viewport->get_is_hovered()) {
        /* Don't let imgui capture input if hovering viewport */
        event.imgui_capture_mouse = false;
        event.imgui_capture_keyboard = false;
    } else {
        event.imgui_capture_mouse = ImGui::GetIO().WantCaptureMouse;
        event.imgui_capture_keyboard = ImGui::GetIO().WantCaptureKeyboard;
    }
}

void ImGuiManager::on_engine_update(const FrameData&) {}

}  // namespace tmt
