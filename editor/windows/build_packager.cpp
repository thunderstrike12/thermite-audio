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

    // get the first unique number combination
    while (std::filesystem::exists(build_folder / (build_name + "_" + std::to_string(counter) + ".zip"))) {
        ++counter;
    }

    zip_path = build_folder / (build_name + "_" + std::to_string(counter) + ".zip");
    return zip_path;
}

std::filesystem::path BuildPackager::find_game_exe() const {
    std::filesystem::path root = get_root_folder();

    for (const auto& entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        if (filename.ends_with(".exe") && filename.starts_with("game_") && filename.find("editor") == std::string::npos) {
            return entry.path();
        }
    }
    return {};
}

bool BuildPackager::create_package() {
    std::filesystem::path root = get_root_folder();
    std::filesystem::path build_folder = get_build_folder();
    std::filesystem::path zip_path = get_unique_zip_path();

    // Folder name matches zip name (without .zip)
    std::filesystem::path output_folder = zip_path;
    output_folder.replace_extension("");  // removes .zip

    // Clean and create output folder
    std::filesystem::remove_all(output_folder);
    std::filesystem::create_directories(output_folder);

    try {
        std::filesystem::copy(find_game_exe(), output_folder, std::filesystem::copy_options::overwrite_existing);
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

    last_built_exe = output_folder / find_game_exe().filename();
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
    // Keep this here, we might need to select a default scene
    // const auto& scenes = tmt::engine.scenes.get_registered_scenes();

    // const char* preview = "Select a scene...";
    // if (const auto it = scenes.find(selected_scene); it != scenes.end()) {
    //     preview = it->second.name.c_str();
    // }

    // ImGui::Text("Scene to package:");
    // if (ImGui::BeginCombo("##SceneSelect", preview)) {
    //     for (const auto& [type_index, scene_info] : scenes) {
    //         const bool is_selected = (selected_scene == type_index);

    //        if (ImGui::Selectable(scene_info.name.c_str(), is_selected)) {
    //            selected_scene = type_index;
    //        }
    //        if (is_selected) ImGui::SetItemDefaultFocus();
    //    }
    //    ImGui::EndCombo();
    //}

    // ImGui::Separator();

    // const bool has_selection = scenes.contains(selected_scene);
    // ImGui::BeginDisabled(!has_selection);
    ImGui::Text("Build Name:");
    ImGui::InputText("##BuildName", &build_name);

    std::filesystem::path output_path = get_unique_zip_path();
    ImGui::TextDisabled("Output: %s", output_path.filename().string().c_str());

    std::filesystem::path game_exe = find_game_exe();
    if (ImGui::CollapsingHeader("Package Contents")) {
        ImGui::BulletText("Exe: %s", game_exe.empty() ? "NOT FOUND" : game_exe.filename().string().c_str());
        ImGui::BulletText("fmod.dll");
        ImGui::BulletText("fmodstudio.dll");
        ImGui::BulletText("engine/");
        ImGui::BulletText("projects/");
        ImGui::BulletText("editor/");
    }

    ImGui::Separator();

    // we use the default scene by default
    const bool has_selection = true;  //(selected_scene != NULL_SCENE);
    const bool has_name = !build_name.empty();
    const bool has_exe = !game_exe.empty();

    const bool can_build = has_selection && has_name && has_exe;
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

    if (!has_exe) {
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Game executable not found!");
    }
}

}  // namespace tmt
