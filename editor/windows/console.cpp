#include "console.hpp"

#include "engine/core/logger.hpp"
using namespace tmt;
void Console::display() { console.DrawContent(); }

void Console::on_editor_start() {
    sink = std::make_shared<ConsoleSink_st>(console.System());
    sink->set_pattern("[%n] %v");
    Log::add_sink(sink);
}

void Console::on_editor_update(const FrameData& time) {}

void Console::on_editor_end() { Log::remove_sink(sink); }
