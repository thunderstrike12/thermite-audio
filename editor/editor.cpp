#if THERMITE_EDITOR
#pragma message(" THERMITE_EDITOR=1 ")
#else
/* Shouldn't happen */
#error THERMITE_EDITOR must be defined to 1 in editor builds
#endif

#include "editor/editor.hpp"

#include "engine/core/logger.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

/* Singleton */
tmt::Editor tmt::editor;

namespace tmt {

Editor::Editor() {}

Editor::~Editor() {}

void Editor::init() { Log::info("Thermite Editor initialized."); }

void Editor::on_engine_init(const ApplicationSpecs&) {
    tmt::Log::info("Starting Thermite Editor...");

    tmt::engine.ecs.create_entity();
}

void Editor::on_engine_update(const FrameData&) { Log::info("Thermite Editor updating..."); }

void Editor::on_engine_fixed_update(const FrameData&) {}

void Editor::on_engine_end() { tmt::Log::info("Shutting down Thermite Editor..."); }

}  // namespace tmt