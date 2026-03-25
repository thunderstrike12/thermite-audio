#include "build_packager.hpp"

#include "editor/font/icon_lookups.hpp"
#include "engine/engine.hpp"
#include "engine/core/scenes.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace tmt {

std::filesystem::path BuildPackager::get_root_folder() const {
    return IO::get_exec_path().parent_path();
}

std::filesystem::path BuildPackager::get_build_folder() const {
    return get_root_folder() / "build";
}

std::filesystem::path BuildPackager::get_unique_zip_path() const {
    std::filesystem::path build_folder = get_build_folder();
    std::filesystem::path zip_path = build_folder / (build_name + ".zip");

    if (!std::filesystem::exists(zip_path)) return zip_path;

    int32_t counter = 1;
    while (std::filesystem::exists(build_folder / (build_name + "_" + std::to_string(counter) + ".zip"))) {
        ++counter;
    }

    zip_path = build_folder / (build_name + "_" + std::to_string(counter) + ".zip");
    return zip_path;
}

void BuildPackager::refresh_available_exes() {
    std::filesystem::path root = get_root_folder();
    available_exes.clear();

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        std::string ext = entry.path().extension().string();
        std::string filename = entry.path().filename().string();

        // Include all .exe files except the editor itself
        if (ext == ".exe" && filename.find("editor") == std::string::npos) {
            available_exes.push_back(entry.path());
        }
    }

    // If the current selection is now out of range, reset it
    if (selected_exe_index >= static_cast<int32_t>(available_exes.size())) {
        selected_exe_index = -1;
    }

    // Auto-select "game.exe" if found, otherwise fall back to single-option auto-select
    if (selected_exe_index < 0) {
        for (int32_t i = 0; i < static_cast<int32_t>(available_exes.size()); ++i) {
            if (available_exes[i].filename() == "game.exe") {
                selected_exe_index = i;
                break;
            }
        }
    }

    if (selected_exe_index < 0 && available_exes.size() == 1) {
        selected_exe_index = 0;
    }
}

std::filesystem::path BuildPackager::get_selected_exe() const {
    if (selected_exe_index >= 0 && selected_exe_index < static_cast<int32_t>(available_exes.size())) {
        return available_exes[selected_exe_index];
    }
    return {};
}

bool BuildPackager::create_package() {
    std::filesystem::path game_exe = get_selected_exe();
    if (game_exe.empty()) {
        status = Status::FAILED;
        status_message = "No executable selected!";
        return false;
    }

    std::filesystem::path root = get_root_folder();
    std::filesystem::path build_folder = get_build_folder();
    std::filesystem::path zip_path = get_unique_zip_path();

    std::filesystem::path output_folder = zip_path;
    output_folder.replace_extension("");

    std::filesystem::remove_all(output_folder);
    std::filesystem::create_directories(output_folder);

    try {
        std::filesystem::copy(game_exe, output_folder, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy(root / "fmod.dll", output_folder, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy(root / "fmodstudio.dll", output_folder, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy(root / "engine", output_folder / "engine", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy(root / "projects", output_folder / "projects", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy(root / "editor", output_folder / "editor", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

        std::string cmd = fmt::format(R"(powershell -Command "Compress-Archive -Path '{}/*' -DestinationPath '{}' -Force")", output_folder.generic_string(), zip_path.generic_string());
        system(cmd.c_str());

    } catch (const std::filesystem::filesystem_error& e) {
        status = Status::FAILED;
        status_message = fmt::format("Failed: {}", e.what());
        Log::error("Packaging failed: {}", e.what());
        return false;
    }

    last_built_exe = output_folder / game_exe.filename();
    status = Status::SUCCESS;
    status_message = fmt::format("Built: {}", output_folder.filename().string());
    Log::info("Package created: {} and {}", output_folder.string(), zip_path.string());
    return true;
}

void BuildPackager::play_build() {
    if (last_built_exe.empty() || !std::filesystem::exists(last_built_exe)) {
        status = Status::FAILED;
        status_message = "No build to play!";
        return;
    }

    std::string cmd = fmt::format(R"(start "" "{}")", last_built_exe.string());
    system(cmd.c_str());
}

void BuildPackager::on_inspect() {
    if (ImGui::Button(ICON_MS_REFRESH " Scan")) {
        refresh_available_exes();
    }

    if (has_scanned == false) {
        refresh_available_exes();
        has_scanned = true;
    }

    ImGui::SameLine();
    ImGui::Text("Game Executable:");

    std::filesystem::path selected_exe = get_selected_exe();

    std::string preview_str = selected_exe.empty() ? "Select an executable..." : selected_exe.filename().string();

    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##ExeSelect", preview_str.c_str())) {
        for (int32_t i = 0; i < static_cast<int32_t>(available_exes.size()); ++i) {
            std::string label = available_exes[i].filename().string();
            const bool is_selected = (selected_exe_index == i);

            if (ImGui::Selectable(label.c_str(), is_selected)) {
                selected_exe_index = i;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (available_exes.empty()) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "No .exe files found in root folder!");
    }

    ImGui::Separator();

    ImGui::Text("Build Name:");
    ImGui::InputText("##BuildName", &build_name);

    std::filesystem::path output_path = get_unique_zip_path();
    ImGui::TextDisabled("Output: %s", output_path.filename().string().c_str());

    if (ImGui::CollapsingHeader("Package Contents")) {
        ImGui::BulletText("Exe: %s", selected_exe.empty() ? "NONE SELECTED" : selected_exe.filename().string().c_str());
        ImGui::BulletText("fmod.dll");
        ImGui::BulletText("fmodstudio.dll");
        ImGui::BulletText("engine/");
        ImGui::BulletText("projects/");
        ImGui::BulletText("editor/");
    }

    ImGui::Separator();

    const bool has_name = !build_name.empty();
    const bool has_exe = !selected_exe.empty();
    const bool can_build = has_name && has_exe;

    ImGui::BeginDisabled(!can_build);
    if (ImGui::Button(ICON_MS_PACKAGE " Build")) {
        create_package();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    ImGui::BeginDisabled(!can_build);
    if (ImGui::Button(ICON_MS_PLAY_ARROW " Build & Play")) {
        if (create_package()) {
            play_build();
        }
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    const bool has_last_build = !last_built_exe.empty() && std::filesystem::exists(last_built_exe);
    ImGui::BeginDisabled(!has_last_build);
    if (ImGui::Button(ICON_MS_PLAY_ARROW " Play Last")) {
        play_build();
    }
    ImGui::EndDisabled();

    if (status != Status::IDLE) {
        ImVec4 color;
        switch (status) {
            case Status::BUILDING:
                color = ImVec4(1, 1, 0, 1);
                break;
            case Status::SUCCESS:
                color = ImVec4(0, 1, 0, 1);
                break;
            case Status::FAILED:
                color = ImVec4(1, 0.4f, 0.4f, 1);
                break;
            default:
                color = ImVec4(1, 1, 1, 1);
        }
        ImGui::TextColored(color, "%s", status_message.c_str());
    }
}

}  // namespace tmt
