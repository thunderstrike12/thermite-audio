#pragma once

#include "engine/tools/serializer.hpp"
#include "editor/core/window.hpp"

namespace tmt {
struct ProfilerSettings {
    bool overwrite = true;
    bool use_timer = false;
    int32_t port = 8086;
    int32_t duration = 10;
    // C style arrays for compatibility
    std::string profiler_path = "tracy.exe";
    std::string capture_path = "tracy-capture.exe";
    std::string output_path = "profile.tracy";
    std::string address = "127.0.0.1";
};
// This implementation is windows specific
class Profiler : public IWindow {
   public:
    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
    void display() override;
    constexpr std::string get_title() const override { return "Profiler"; }
};

}  // namespace tmt
JSON_REFLECT(tmt::ProfilerSettings, profiler_path, capture_path, output_path, address, port, overwrite, use_timer, duration);
