#include "screenshot.hpp"

#include "engine/core/renderer/renderer.hpp"

namespace tmt {

std::filesystem::path resolve_path(const ScreenshotSettings& s) {
    std::string name = s.filename;

    name += ".png";
    return std::filesystem::absolute(std::filesystem::path(s.directory) / name);
}

void ScreenshotWindow::on_inspect() {
    ScreenshotSettings& s = engine.renderer.screenshot_settings;

    ImGui::InputText("Screenshot Directory", &s.directory);
    if (ImGui::Button(ICON_MS_OPEN_IN_NEW " Open In File Explorer")) {
        // We use "string()" instead of "generic_string()" because it automatically keeps the file separators consistent which is necessary for this command.
        const std::string open_command = std::format(R"(explorer.exe "{}")", std::filesystem::absolute(std::filesystem::path(s.directory)).string());

        system(open_command.c_str());
    }

    ImGui::InputText("Screenshot Name", &s.filename);

    /* clang-format off */
    static constexpr const char* labels[] = {
        "Match Viewport", 
        "720p  (1280 x 720)", 
        "1080p (1920 x 1080)", 
        "1440p (2560 x 1440)", 
        "4K    (3840 x 2160)", 
        "8K    (7680 x 4320)",
    };
    /* clang-format on */

    int idx = (int)s.resolution_preset;
    if (ImGui::Combo("Resolution", &idx, labels, IM_ARRAYSIZE(labels))) {
        s.resolution_preset = (ScreenshotSettings::CaptureResolution)idx;

        switch (s.resolution_preset) {
            case ScreenshotSettings::CaptureResolution::MatchViewport:
                s.width = engine.renderer.render_view.gpu_view.resolution.x;
                s.height = engine.renderer.render_view.gpu_view.resolution.y;
                break;
            case ScreenshotSettings::CaptureResolution::HD_720p:
                s.width = 1280;
                s.height = 720u;
                break;
            case ScreenshotSettings::CaptureResolution::FullHD_1080p:
                s.width = 1920u;
                s.height = 1080u;
                break;
            case ScreenshotSettings::CaptureResolution::QHD_1440p:
                s.width = 2560u;
                s.height = 1440u;
                break;
            case ScreenshotSettings::CaptureResolution::UHD_4K:
                s.width = 3840u;
                s.height = 2160u;
                break;
            case ScreenshotSettings::CaptureResolution::UHD_8K:
                s.width = 7680u;
                s.height = 4320u;
                break;
        }
    }

    ImGui::Checkbox("Include UI", &s.include_ui);

    ImGui::Separator();

    // Preview of the resolved path
    ImGui::TextDisabled("Will save to: %s", resolve_path(s).string().c_str());

    ImGui::Separator();

    if (ImGui::Button("Take Screenshot", ImVec2(-1, 0))) {
        s.request_capture = true;
        s.warming_up = true;
    }
}

}  // namespace tmt
