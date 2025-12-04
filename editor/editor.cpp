#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
/* Shouldn't happen */
#error THERMITE_EDITOR must be defined to 1 in editor builds
#endif

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include "editor/editor.hpp"

#include "engine/engine.hpp"

#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/window.hpp"

#include "engine/events/sdl.hpp"

/* Singleton */
tmt::Editor tmt::editor;

namespace tmt {

class ImGuiUpdate : public internal::OnSdlEvent {
   public:
    ImGuiUpdate() : tmt::internal::OnSdlEvent() { priority = 100; }

   private:
    void on_sdl_event(SDL_Event& event) override { ImGui_ImplSDL3_ProcessEvent(&event); }
};

void Editor::init() { Log::info("Thermite Editor initialized."); }

void Editor::on_engine_init(const ApplicationSpecs&) {
    static ImGuiUpdate imgui_event_listener {};

    tmt::Log::info("Starting Thermite Editor...");

    tmt::engine.ecs.create_entity();

    /* Initialize the immediate mode GUI */
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForVulkan(engine.window.window);
    tmt::engine.renderer.set_imgui(&imgui, IMGUI_FUNCTIONS);
}

void Editor::on_engine_update(const FrameData&) {
    /* Start a new imgui frame */
    imgui.new_frame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Camera");
    ImGui::End();

    /* End the imgui frame */
    ImGui::Render();
}

void Editor::on_engine_fixed_update(const FrameData&) {}

void Editor::on_engine_end() {
    tmt::Log::info("Shutting down Thermite Editor...");
    imgui.deinit();
}

}  // namespace tmt