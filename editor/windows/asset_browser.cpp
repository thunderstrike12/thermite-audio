#include "asset_browser.hpp"

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/resources/voxel_scene.hpp"
#include "engine/tools/file_dialog.hpp"
#include "engine/tools/svh_format.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include <magic_enum/magic_enum.hpp>

#include "engine/engine.hpp"
#include "engine/core/resources.hpp"
#include "engine/tools/prefab_helper.hpp"
#include "engine/core/renderer/renderer.hpp"

#include "editor/editor.hpp"

#include "node_hierarchy.hpp"

namespace {

struct FileDropState {
    bool is_dropping { false };
    std::vector<tmt::IO::FileLocation> dropped_files;
} file_drop_state;

std::atomic_flag block_import_atomic {};

template <typename ItemType, typename ContainerType>
void apply_requests(ImGuiMultiSelectIO* io, std::vector<ItemType>& selection, const ContainerType& items) {
    for (const auto& request : io->Requests) {
        switch (request.Type) {
            case ImGuiSelectionRequestType_None:
                break;

            case ImGuiSelectionRequestType_SetAll:
                selection.clear();
                if (request.Selected) selection.insert(selection.begin(), std::begin(items), std::end(items));
                break;

            case ImGuiSelectionRequestType_SetRange: {
                if (request.Selected) {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++) {
                        selection.push_back(items.at(i));
                    }
                } else {
                    for (ImGuiSelectionUserData i = request.RangeFirstItem; i <= request.RangeLastItem; i++) {
                        selection.erase(std::ranges::find(selection, items.at(i)));
                    }
                }
            } break;
        }
    }
}

[[nodiscard]] bool path_valid(const std::filesystem::path& path) {
    return !path.empty() && exists(path);
}

// Generate an unused filename by appending "(NUMBER)" after the filename.
tmt::IO::FileLocation find_unused_file_location(tmt::IO::FileLocation file_location) {
    if (!tmt::IO::file_exists(file_location)) return file_location;

    const std::string& stem_string = file_location.relative_path.stem().generic_string();
    const auto& extension = file_location.relative_path.extension();

    size_t index = 0;
    tmt::IO::FileLocation new_location = file_location;
    do {
        ++index;
        new_location = file_location;
        new_location.relative_path.replace_filename(stem_string + fmt::format("({})", index)) += extension;
    } while (tmt::IO::file_exists(new_location));

    return new_location;
}

void drag_drop_file(const tmt::IO::FileLocation& location) {
    if (!ImGui::BeginDragDropSource()) return;

    const std::string& location_json = tmt::Serializer::serialize(location).dump(4);
    ImGui::SetDragDropPayload("FileLocation", location_json.data(), location_json.size());

    ImGui::Text("sub_location: %s", magic_enum::enum_name(location.sub_location).data());
    ImGui::Text("relative_path: %s", location.relative_path.generic_string().c_str());

    ImGui::EndDragDropSource();
}

void drag_drop_directory(const tmt::IO::FileLocation& location, const bool is_window = false) {
    if (!is_window) {
        if (!ImGui::BeginDragDropTarget()) return;
    } else {
        if (!ImGui::BeginDragDropTargetCustom(ImGui::GetCurrentWindow()->Rect(), ImGui::GetID("WindowDragDropTarget"))) return;
    }

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Import");
    if (payload != nullptr) {
        for (const tmt::IO::FileLocation& file_location : file_drop_state.dropped_files) {
            tmt::AssetBrowser::import_asset(file_location, location);
        }
        file_drop_state.dropped_files.clear();
    }

    ImGui::EndDragDropTarget();
}

std::filesystem::path path_to_lower(const std::filesystem::path& path) {
    std::string path_string = path.generic_string();
    for (char& character : path_string) {
        character = static_cast<char>(std::tolower(character));
    }

    return std::filesystem::path { path_string };
}

}  // namespace

namespace tmt {

void AssetBrowser::recurse_parse_directory(Directory& directory) {
    try {
        std::filesystem::directory_iterator iterator { directory.location.get_relative_path() };
        for (const auto& entry : iterator) {
            if (!entry.is_directory()) continue;

            Directory& sub_directory = directory.sub_directories.emplace_back();
            sub_directory.location = { directory.location.sub_location, directory.location.relative_path / entry.path().filename() };
            recurse_parse_directory(sub_directory);
        }
    } catch (const std::exception& e) {
        Log::error("Error with parsing directory: {}", e.what());
    }
}

void AssetBrowser::on_sdl_event(internal::SdlEvent& event) {
    const SDL_DropEvent& drop_event = event.event.drop;

    switch (drop_event.type) {
        case SDL_EVENT_DROP_BEGIN:
            file_drop_state.is_dropping = true;
            break;

        case SDL_EVENT_DROP_FILE:
            file_drop_state.dropped_files.push_back(IO::path_to_file_location(drop_event.data));
            break;

        case SDL_EVENT_DROP_COMPLETE:
            file_drop_state.is_dropping = false;
            break;

        default:
            break;
    }
}

void AssetBrowser::update_bookmark_vector(std::vector<Bookmark>& bookmarks) {
    for (auto& bookmark : bookmarks) {
        if (!bookmark.location_watcher.is_valid()) {              // If the location watcher is invalid, make it valid.
            bookmark.location_watcher = DirectoryWatcher { bookmark.directory.location, true, true, WatchReason::VISUAL };
        } else if (!bookmark.location_watcher.check_changes()) {  // If the watcher is valid skip updating the bookmark view if there were no updates to the directory.
            continue;
        }

        bookmark.directory.sub_directories.clear();
        recurse_parse_directory(bookmark.directory);
    }
}

bool AssetBrowser::location_is_bookmarked(const IO::FileLocation& location) {
    if (location.relative_path.empty()) return true;

    const auto& bookmark_iterator = std::ranges::find_if(bookmarks, [&location](const Bookmark& bookmark) { return bookmark.directory.location == location; });
    return bookmark_iterator != bookmarks.end();
}

void AssetBrowser::on_inspect() {
    // If we are dragging a file in from another program we manually create a drag and drop source to use in ImGui.
    if (file_drop_state.is_dropping) {
        ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern);  // ImGuiDragDropFlags_SourceExtern means it'll always return true.

        ImGui::SetDragDropPayload("Import", nullptr, 0);
        ImGui::Text("Import files...");

        ImGui::EndDragDropSource();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);
    const bool menu_bar_open = ImGui::BeginMenuBar();
    ImGui::PopStyleVar();
    if (menu_bar_open) {
        /* Navigation buttons */
        ImGui::BeginDisabled(undo_viewing_locations.empty());
        if (ImGui::Button(ICON_MS_ARROW_BACK)) move_location_stacks(undo_viewing_locations, redo_viewing_locations);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(redo_viewing_locations.empty());
        if (ImGui::Button(ICON_MS_ARROW_FORWARD)) move_location_stacks(redo_viewing_locations, undo_viewing_locations);
        ImGui::EndDisabled();

        const std::filesystem::path& current_path = viewing_location.relative_path;
        ImGui::BeginDisabled(current_path.empty());
        if (ImGui::Button(ICON_MS_ARROW_UPWARD)) pending_viewing_location = { viewing_location.sub_location, current_path.parent_path() };
        ImGui::EndDisabled();

        if (ImGui::Button(ICON_MS_REFRESH)) pending_viewing_location = viewing_location;

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

        /* Breadcrumb navigation */
        display_directory_bar();

        /* Right-aligned items */
        ImGui::Spacing();

        ImGui::BeginDisabled(location_is_bookmarked(viewing_location));
        if (ImGui::Button(ICON_MS_BOOKMARK_ADD)) {
            std::string display_name = viewing_location.get_absolute_path().stem().generic_string();
            bookmarks.emplace_back(std::move(display_name), Directory { viewing_location });
        }
        ImGui::EndDisabled();

        ImGui::EndMenuBar();
    }

    if (!ImGui::BeginTable("AssetBrowserTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) return;

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Bookmarks")) {
        display_bookmarks();
    }
    ImGui::EndChild();

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Files")) {
        display_viewing_location();
    }
    drag_drop_directory(viewing_location, true);
    ImGui::EndChild();

    ImGui::EndTable();
}

void AssetBrowser::on_editor_start() {
    const IO::FileLocation project_asset_location { IO::Location::PROJECT, "" };
    default_bookmarks.emplace_back("Project", Directory { project_asset_location });

    const IO::FileLocation engine_asset_location { IO::Location::ENGINE, "" };
    default_bookmarks.emplace_back("Engine", Directory { engine_asset_location });

    const IO::FileLocation editor_asset_location { IO::Location::EDITOR, "" };
    default_bookmarks.emplace_back("Editor", Directory { editor_asset_location });
}

void AssetBrowser::on_editor_update(const FrameData&) {
    block_import_atomic.wait(true);

    // Check if there are changes in the viewing directory, if so we update the viewing directory by setting it to pending.
    if (viewing_location_watcher.is_valid() && viewing_location_watcher.check_changes()) pending_viewing_location = viewing_location;

    update_location_history();

    // Update the default bookmarks (asset dirs for: engine, editor and project).
    update_bookmark_vector(default_bookmarks);
    // Update the user added bookmarks.
    update_bookmark_vector(bookmarks);
}

void AssetBrowser::import_asset(const IO::FileLocation& import_file, const IO::FileLocation& location) {
    const std::string& extension = import_file.relative_path.extension().generic_string();
    if (extension == ".vengi") {
        const auto vengi_scene = engine.resources.load_resource<VoxelScene>(import_file);
        if (!vengi_scene) {
            Log::error("Failed to import asset: failed to load .vengi file.");
            return;
        }

        IO::FileLocation import_file_location = location;
        import_file_location.relative_path /= import_file.relative_path.filename().replace_extension(".svh");
        import_file_location = find_unused_file_location(import_file_location);

        const std::vector<char>& data = encode_svh(vengi_scene.resource->root_nodes);
        IO::write_file(import_file_location, data.data(), data.size());

        return;
    }

    Log::error("Failed to import asset: invalid file type.");
}

void AssetBrowser::import_assets_dialog() const {
    open_files_dialog(
        [this](const std::vector<IO::FileLocation>& file_locations) {
            block_import_atomic.test_and_set();
            for (const auto& file_location : file_locations) {
                import_asset(file_location, viewing_location);
            }
            block_import_atomic.clear();
            block_import_atomic.notify_one();
        },
        { { "Vengi voxel object", "vengi" } }
    );
}

void AssetBrowser::update_location_history() {
    if (pending_viewing_location.has_value()) {
        // Clear the redo history if the user manually went to a new directory.
        redo_viewing_locations = {};
        if (*pending_viewing_location != viewing_location) undo_viewing_locations.push(viewing_location);
        update_viewing_locations();
    }
}

void AssetBrowser::update_viewing_locations() {
    if (!pending_viewing_location.has_value()) return;

    const std::filesystem::path& path = pending_viewing_location->relative_path;
    if (!path.empty() && path.filename().empty()) pending_viewing_location->relative_path = path.parent_path();

    if (!exists(pending_viewing_location->get_relative_path())) {
        Log::warn(Log::Scope::EDITOR, "AssetBrowser trying to switch to invalid directory: {}", pending_viewing_location->get_relative_path().generic_string());
        pending_viewing_location.reset();
        return;
    }

    viewing_location = *pending_viewing_location;
    pending_viewing_location.reset();

    selected_locations.clear();
    viewing_locations.clear();

    const std::filesystem::directory_iterator directory_iterator { viewing_location.get_relative_path() };
    for (const std::filesystem::directory_entry& entry : directory_iterator) {
        viewing_locations.emplace_back(viewing_location.sub_location, relative(entry.path(), IO::get_sub_location_path(viewing_location.sub_location)));
    }

    // Sort the paths by name but make sure that directories are always first (just like how file explorer does it).
    std::ranges::sort(viewing_locations, [](const IO::FileLocation& a, const IO::FileLocation& b) {
        const std::filesystem::path a_path = path_to_lower(a.get_relative_path());
        const std::filesystem::path b_path = path_to_lower(b.get_relative_path());

        if (is_directory(a_path) == is_directory(b_path)) return a_path < b_path;
        return is_directory(a_path);
    });

    viewing_location_watcher = DirectoryWatcher { viewing_location, false, true, WatchReason::VISUAL };
}

void AssetBrowser::move_location_stacks(std::stack<IO::FileLocation>& from, std::stack<IO::FileLocation>& to) {
    to.push(viewing_location);
    pending_viewing_location = from.top();
    from.pop();

    update_viewing_locations();
}

void AssetBrowser::display_directory_bar() {
    static std::string viewing_dir_name;

    const float margin_width = ImGui::CalcTextSize(ICON_MS_BOOKMARK).x + (ImGui::GetWindowWidth() / 5.0f);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - margin_width);
    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InputText("##ViewingDirEdit", &viewing_dir_name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_ElideLeft)) {
        IO::FileLocation new_location { viewing_location.sub_location, viewing_dir_name };

        if (!is_directory(new_location.get_relative_path())) new_location.relative_path = new_location.relative_path.parent_path();
        if (path_valid(new_location.get_relative_path())) pending_viewing_location = new_location;

        viewing_dir_name.clear();
    }

    if (ImGui::IsItemActive() || ImGui::GetItemRectSize().x <= 0.0f) return;
    if (ImGui::IsItemDeactivated()) viewing_dir_name.clear();

    const ImGuiID viewing_dir_edit_id = ImGui::GetItemID();
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());

    ImGui::SetNextItemAllowOverlap();
    if (ImGui::InvisibleButton("##InputSelect", ImGui::GetItemRectSize())) {
        viewing_dir_name = viewing_location.relative_path.generic_string();

        // Activate (start editing) the text input widget if the invisible button is clicked (button is necessary to set the input text).
        ImGui::ActivateItemByID(viewing_dir_edit_id);
    }
    const ImVec2 cursor_end_position = ImGui::GetCursorPos();

    // Adjust to get the text in the middle of the InputText().
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin() + ImVec2 { ImGui::GetStyle().FramePadding.x, 0.0f });

    // Add the sub_path button which has to be handled separately.
    const std::string sub_path_name = IO::get_sub_location_path(viewing_location.sub_location).stem().generic_string();
    if (ImGui::SmallButton(sub_path_name.c_str())) pending_viewing_location = { viewing_location.sub_location, "" };

    // Add the ">" for the sub_path button.
    const bool valid_relative_path = !viewing_location.relative_path.empty();
    if (valid_relative_path) ImGui::Text(">");

    std::filesystem::path intermediate_dir;
    for (const std::filesystem::path& sub_directory : viewing_location.relative_path) {
        intermediate_dir /= sub_directory;

        if (ImGui::SmallButton(sub_directory.generic_string().c_str())) pending_viewing_location = { viewing_location.sub_location, intermediate_dir };

        if (viewing_location.relative_path != intermediate_dir) ImGui::Text(">");
    }

    // Reset ImGui cursor to allow us to continue adding widgets.
    ImGui::SetCursorPos(cursor_end_position);
}

ImGuiID AssetBrowser::recurse_display_bookmark_dirs(const Directory& directory, const std::string& display_name) {
    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

    const std::string& directory_name = (display_name.empty() ? directory.location.get_relative_path().filename().generic_string() : display_name);
    const ImGuiID node_id = ImGui::GetID(directory_name.c_str());  // Generate node id ourselves to get some info about the id later.

    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (directory.sub_directories.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    // Checks for context menu popup being open (has the same id as the node), lets us draw the node as selected.
    flags |= (ImGui::IsPopupOpen(node_id, ImGuiPopupFlags_None) ? ImGuiTreeNodeFlags_Selected : 0);

    // Check ImGui internal storage to check if the node is already toggled open, this allows us to set the display label based on if the folder is toggled open.
    const std::string node_display_text = (ImGui::GetStateStorage()->GetBool(node_id) ? ICON_MS_FOLDER_OPEN " " : ICON_MS_FOLDER " ") + directory_name;
    const bool node_open = ImGui::TreeNodeBehavior(node_id, flags, node_display_text.c_str(), nullptr);

    drag_drop_directory(directory.location);

    location_context_menu(directory.location, true);
    if (ImGui::IsItemClicked()) pending_viewing_location = directory.location;

    if (node_open) {
        for (const auto& sub_directory : directory.sub_directories) {
            recurse_display_bookmark_dirs(sub_directory);
        }

        ImGui::TreePop();
    }

    return node_id;
}

void AssetBrowser::location_context_menu(const IO::FileLocation& location, const bool allow_bookmark) {
    constexpr ImGuiPopupFlags flags = ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
    if (!ImGui::BeginPopupContextItem(nullptr, flags)) return;

    // Open the folder in file explorer and select the specific file.
    if (ImGui::MenuItem(ICON_MS_OPEN_IN_NEW " Show in File Explorer")) {
        // We use "string()" instead of "generic_string()" because it automatically keeps the file separators consistent which is necessary for this command.
        const std::string show_command = std::format(R"(explorer.exe /select,"{}")", location.get_absolute_path().string());
        system(show_command.c_str());
    }

    if (ImGui::BeginMenu(ICON_MS_ASSIGNMENT " Copy as path")) {
        if (ImGui::MenuItem("Relative")) {
            const std::filesystem::path generic_path { location.get_relative_path(), std::filesystem::path::generic_format };
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }
        if (ImGui::MenuItem("Absolute")) {
            const std::filesystem::path generic_path { location.get_absolute_path(), std::filesystem::path::generic_format };
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }

        ImGui::EndMenu();
    }

    const std::filesystem::path ext = location.get_absolute_path().extension();

    // Shortcut for opening '.svh' files inside the voxel editor.
    if (ext == ".svh") {
        if (ImGui::MenuItem(ICON_MS_OPEN_JAM " Open in Voxel Editor")) {
            editor.switch_mode(Editor::Mode::VOXEL, location);
        }
    }

    // Shortcut for importing '.vengi' files.
    if (ext == ".vengi") {
        if (ImGui::MenuItem(ICON_MS_DOWNLOAD " Import as SVH")) {
            import_asset(location, viewing_location);
        }
    }

    ImGui::BeginDisabled(location_is_bookmarked(location));
    if (allow_bookmark && ImGui::MenuItem(ICON_MS_BOOKMARK_ADD " Add Bookmark")) {
        std::string display_name = location.get_absolute_path().stem().generic_string();
        bookmarks.emplace_back(std::move(display_name), Directory { location });
    }
    ImGui::EndDisabled();

    ImGui::EndPopup();
}

void AssetBrowser::viewing_context_menu() const {
    constexpr ImGuiPopupFlags flags = ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight;
    if (!ImGui::BeginPopupContextWindow(nullptr, flags)) return;

    if (ImGui::MenuItem(ICON_MS_OPEN_IN_NEW " Open In File Explorer")) {
        // We use "string()" instead of "generic_string()" because it automatically keeps the file separators consistent which is necessary for this command.
        const std::string open_command = std::format(R"(explorer.exe "{}")", viewing_location.get_absolute_path().string());

        system(open_command.c_str());
    }

    if (ImGui::BeginMenu(ICON_MS_ASSIGNMENT " Copy Current Path")) {
        if (ImGui::MenuItem("Relative")) {
            const std::filesystem::path generic_path { viewing_location.get_relative_path(), std::filesystem::path::generic_format };
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }
        if (ImGui::MenuItem("Absolute")) {
            const std::filesystem::path generic_path { viewing_location.get_absolute_path(), std::filesystem::path::generic_format };
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }

        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

void AssetBrowser::display_bookmarks() {
    for (const auto& bookmark : default_bookmarks) {
        recurse_display_bookmark_dirs(bookmark.directory, bookmark.display_name);
    }

    ImGui::SeparatorText(ICON_MS_BOOKMARKS " Bookmarks");

    int64_t i = 0;
    for (const auto& bookmark : bookmarks) {
        const std::string absolute_path_string = bookmark.directory.location.get_absolute_path().generic_string();

        ImGui::PushID(absolute_path_string.c_str());
        const ImGuiID node_id = recurse_display_bookmark_dirs(bookmark.directory);
        ImGui::PopID();

        //
        if (ImGui::BeginPopupEx(node_id, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 3.0f);
            if (ImGui::MenuItem(ICON_MS_BOOKMARK_REMOVE " Remove Bookmark")) bookmarks.erase(bookmarks.begin() + i);

            ImGui::EndPopup();
        }
        ++i;
    }
}

void AssetBrowser::display_viewing_location() {
    if (!path_valid(viewing_location.get_relative_path())) return;

    constexpr ImGuiMultiSelectFlags flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect2d | ImGuiMultiSelectFlags_ClearOnClickVoid |
                                            ImGuiMultiSelectFlags_ScopeWindow | ImGuiMultiSelectFlags_SelectOnClickRelease;

    ImGuiMultiSelectIO* select_io = ImGui::BeginMultiSelect(flags, static_cast<int>(selected_locations.size()), static_cast<int>(viewing_locations.size()));
    apply_requests(select_io, selected_locations, viewing_locations);

    const bool is_playing = engine.game_controller.is_playing();

    ImS64 i = 0;
    for (const IO::FileLocation& location : viewing_locations) {
        const bool location_is_directory = is_directory(location.get_relative_path());
        const std::string location_name = (location_is_directory ? ICON_MS_FOLDER " " : ICON_MS_DESCRIPTION " ") + location.relative_path.filename().generic_string();

        const bool is_selected = std::ranges::find(selected_locations, location) != selected_locations.end();

        ImGui::SetNextItemSelectionUserData(i);
        ImGui::Selectable(location_name.c_str(), is_selected);
        location_context_menu(location, location_is_directory);
        if (location_is_directory) {
            drag_drop_directory(location);
        } else {
            drag_drop_file(location);
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (location_is_directory) {
                pending_viewing_location = location;
            } else if (is_playing == false && location.get_relative_path().extension() == PrefabHelper::Config::PREFAB_EXTENSION) {
                editor.switch_mode(Editor::Mode::PREFAB, location);
            } else if (is_playing == false && location.get_relative_path().extension() == ".svh") {
                editor.switch_mode(Editor::Mode::VOXEL, location);
            } else {
                const std::string open_file_command = std::format(R"(start "" "{}")", location.get_relative_path().generic_string());
                if (system(open_file_command.c_str()) != 0) Log::warn(Log::Scope::ENGINE, "Couldn't open file.");
            }
        }
        ++i;
    }

    select_io = ImGui::EndMultiSelect();
    apply_requests(select_io, selected_locations, viewing_locations);

    viewing_context_menu();
}

}  // namespace tmt