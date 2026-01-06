#include "asset_browser.hpp"

#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

#include <magic_enum/magic_enum.hpp>

namespace {

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

// Returns a malloc-ed buffer of sequential IO::FileLocation data (caller needs to manually call std::free() on the buffer).
void* file_location_to_buffer(const tmt::IO::FileLocation& location, size_t& size) {
    constexpr size_t sub_location_size = sizeof(location.sub_location);

    const std::string& path_string = location.relative_path.generic_string();
    const size_t relative_path_size = path_string.size();

    size = sub_location_size + relative_path_size;
    void* data = std::malloc(size);  // Allocate enough data to hold teh IO::Location and the relative path string.

    *static_cast<tmt::IO::Location*>(data) = location.sub_location;
    std::memcpy(static_cast<char*>(data) + sub_location_size, path_string.data(), relative_path_size);

    return data;
}

void drag_drop_location(const tmt::IO::FileLocation& location) {
    if (!ImGui::BeginDragDropSource()) return;

    // Create a trivially copyable version of the IO::FileLocation data, this way we can use it as ImGUI drag/drop data.
    size_t size;
    void* data = file_location_to_buffer(location, size);
    ImGui::SetDragDropPayload("FileLocation", data, size);
    std::free(data);

    ImGui::Text("sub_location: %s", magic_enum::enum_name(location.sub_location).data());
    ImGui::Text("relative_path: %s", location.relative_path.generic_string().c_str());

    ImGui::EndDragDropSource();
}

}  // namespace

namespace tmt {

void AssetBrowser::recurse_parse_directory(Directory& directory) {
    std::filesystem::directory_iterator iterator {directory.location.get_relative_path()};
    for (const auto& entry : iterator) {
        if (!entry.is_directory()) continue;

        Directory& sub_directory = directory.sub_directories.emplace_back();
        sub_directory.location = {directory.location.sub_location, directory.location.relative_path / entry.path().filename()};
        recurse_parse_directory(sub_directory);
    }
}

[[nodiscard]] bool path_valid(const std::filesystem::path& path) { return !path.empty() && exists(path); }

void AssetBrowser::update_bookmark_vector(std::vector<Bookmark>& bookmarks) {
    for (auto& bookmark : bookmarks) {
        if (!bookmark.location_watcher.is_valid()) {  // If the location watcher is invalid, make it valid.
            bookmark.location_watcher = tmt::DirectoryWatcher {bookmark.directory.location};
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

void AssetBrowser::display() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding);
    const bool menu_bar_open = ImGui::BeginMenuBar();
    ImGui::PopStyleVar();
    if (menu_bar_open) {
        ImGui::BeginDisabled(undo_viewing_locations.empty());
        if (ImGui::Button(ICON_MS_ARROW_BACK)) move_location_stacks(undo_viewing_locations, redo_viewing_locations);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(redo_viewing_locations.empty());
        if (ImGui::Button(ICON_MS_ARROW_FORWARD)) move_location_stacks(redo_viewing_locations, undo_viewing_locations);
        ImGui::EndDisabled();

        const std::filesystem::path& current_path = viewing_location.relative_path;
        ImGui::BeginDisabled(current_path.empty());
        if (ImGui::Button(ICON_MS_ARROW_UPWARD_ALT)) pending_viewing_location = {viewing_location.sub_location, current_path.parent_path()};
        ImGui::EndDisabled();

        if (ImGui::Button(ICON_MS_AUTORENEW)) pending_viewing_location = viewing_location;

        ImGui::Text("Directory:");
        display_directory_bar();

        ImGui::BeginDisabled(location_is_bookmarked(viewing_location));
        if (ImGui::Button(ICON_MS_BOOKMARK_ADD)) bookmarks.emplace_back(Directory {viewing_location});
        ImGui::EndDisabled();

        ImGui::EndMenuBar();
    }

    // Adds a line under the MenuBar to make it look *slightly* nicer.
    ImGui::SetCursorScreenPos(ImVec2 {ImGui::GetCursorScreenPos().x, ImGui::GetItemRectMax().y});
    ImGui::Separator();

    const float available_width = ImGui::GetContentRegionAvail().x;
    if (!ImGui::BeginTable("AssetBrowserTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable)) return;

    ImGui::TableSetupColumn("###Default folders", ImGuiTableColumnFlags_WidthFixed, available_width / 6.0f);

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Bookmarks")) {
        display_bookmarks();
    }
    ImGui::EndChild();

    ImGui::TableNextColumn();
    if (ImGui::BeginChild("Files")) {
        display_viewing_location();
    }
    ImGui::EndChild();

    ImGui::EndTable();
}

void AssetBrowser::on_editor_start() {
    constexpr std::array bookmark_paths = magic_enum::enum_entries<IO::Location>();
    for (const auto& [location, name] : bookmark_paths) {
        const IO::FileLocation file_location {location, ""};
        default_bookmarks.emplace_back(Directory {file_location});
    }
}

void AssetBrowser::on_editor_update(const FrameData&) {
    // Check if there are changes in the viewing directory, if so we update the viewing directory by setting it to pending.
    if (viewing_location_watcher.is_valid() && viewing_location_watcher.check_changes()) pending_viewing_location = viewing_location;

    update_location_history();

    // Update the default bookmarks (asset dirs for: engine, editor and project).
    update_bookmark_vector(default_bookmarks);
    // Update the user added bookmarks.
    update_bookmark_vector(bookmarks);
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

    const std::filesystem::directory_iterator directory_iterator {viewing_location.get_relative_path()};
    for (const std::filesystem::directory_entry& entry : directory_iterator) {
        viewing_locations.emplace_back(viewing_location.sub_location, relative(entry.path(), IO::get_sub_location_path(viewing_location.sub_location)));
    }

    // Sort the paths by name but make sure that directories are always first (just like how file explorer does it).
    std::ranges::sort(viewing_locations, [](const IO::FileLocation& a, const IO::FileLocation& b) {
        const std::filesystem::path& a_path = a.get_relative_path();
        const std::filesystem::path& b_path = b.get_relative_path();

        if (is_directory(a_path) == is_directory(b_path)) return a_path < b_path;
        return is_directory(a_path);
    });

    viewing_location_watcher = DirectoryWatcher {viewing_location, false};
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
        IO::FileLocation new_location {viewing_location.sub_location, viewing_dir_name};

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
    ImGui::SetCursorScreenPos(ImGui::GetItemRectMin() + ImVec2 {ImGui::GetStyle().FramePadding.x, 0.0f});

    // Add the sub_path button which has to be handled separately.
    const std::string sub_path_name = IO::get_sub_location_path(viewing_location.sub_location).stem().generic_string();
    if (ImGui::SmallButton(sub_path_name.c_str())) pending_viewing_location = {viewing_location.sub_location, ""};

    // Add the ">" for the sub_path button.
    const bool valid_relative_path = !viewing_location.relative_path.empty();
    if (valid_relative_path) ImGui::Text(">");

    std::filesystem::path intermediate_dir;
    for (const std::filesystem::path& sub_directory : viewing_location.relative_path) {
        intermediate_dir /= sub_directory;

        if (ImGui::SmallButton(sub_directory.generic_string().c_str())) pending_viewing_location = {viewing_location.sub_location, intermediate_dir};

        if (viewing_location.relative_path != intermediate_dir) ImGui::Text(">");
    }

    // Reset ImGui cursor to allow us to continue adding widgets.
    ImGui::SetCursorPos(cursor_end_position);
}

ImGuiID AssetBrowser::recurse_display_bookmark_dirs(const Directory& directory) {
    constexpr ImGuiTreeNodeFlags default_flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

    const std::string& directory_path = directory.location.get_relative_path().filename().generic_string();
    const ImGuiID node_id = ImGui::GetID(directory_path.c_str());  // Generate node id ourselves to get some info about the id later.

    ImGuiTreeNodeFlags flags = default_flags;
    flags |= (directory.sub_directories.empty() ? ImGuiTreeNodeFlags_Leaf : 0);
    // Checks for context menu popup being open (has the same id as the node), lets us draw the node as selected.
    flags |= (ImGui::IsPopupOpen(node_id, ImGuiPopupFlags_None) ? ImGuiTreeNodeFlags_Selected : 0);

    // Check ImGui internal storage to check if the node is already toggled open, this allows us to set the display label based on if the folder is toggled open.
    const std::string node_display_text = (ImGui::GetStateStorage()->GetBool(node_id) ? ICON_MS_FOLDER_OPEN " " : ICON_MS_FOLDER " ") + directory_path;
    const bool node_open = ImGui::TreeNodeBehavior(node_id, flags, node_display_text.c_str(), nullptr);

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
    if (ImGui::MenuItem(ICON_MS_OPEN_IN_NEW " Show In File Explorer")) {
        // We use "string()" instead of "generic_string()" because it automatically keeps the file separators consistent which is necessary for this command.
        const std::string show_command = std::format(R"(explorer.exe /select,"{}")", location.get_absolute_path().string());
        system(show_command.c_str());
    }

    if (ImGui::BeginMenu(ICON_MS_ASSIGNMENT " Copy As Path")) {
        if (ImGui::MenuItem("Relative")) {
            const std::filesystem::path generic_path {location.get_relative_path(), std::filesystem::path::generic_format};
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }
        if (ImGui::MenuItem("Absolute")) {
            const std::filesystem::path generic_path {location.get_absolute_path(), std::filesystem::path::generic_format};
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }

        ImGui::EndMenu();
    }

    ImGui::BeginDisabled(location_is_bookmarked(location));
    if (allow_bookmark && ImGui::MenuItem(ICON_MS_BOOKMARK_ADD " Add Bookmark")) bookmarks.emplace_back(Directory {location});
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
            const std::filesystem::path generic_path {viewing_location.get_relative_path(), std::filesystem::path::generic_format};
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }
        if (ImGui::MenuItem("Absolute")) {
            const std::filesystem::path generic_path {viewing_location.get_absolute_path(), std::filesystem::path::generic_format};
            ImGui::SetClipboardText(generic_path.generic_string().c_str());
        }

        ImGui::EndMenu();
    }

    ImGui::EndPopup();
}

void AssetBrowser::display_bookmarks() {
    for (const auto& bookmark : default_bookmarks) {
        recurse_display_bookmark_dirs(bookmark.directory);
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

    ImS64 i = 0;
    for (const IO::FileLocation& location : viewing_locations) {
        const bool location_is_directory = is_directory(location.get_relative_path());
        const std::string location_name = (location_is_directory ? ICON_MS_FOLDER " " : ICON_MS_DESCRIPTION " ") + location.relative_path.filename().generic_string();

        const bool is_selected = std::ranges::find(selected_locations, location) != selected_locations.end();

        ImGui::SetNextItemSelectionUserData(i);
        ImGui::Selectable(location_name.c_str(), is_selected);
        location_context_menu(location, location_is_directory);
        if (!location_is_directory) drag_drop_location(location);

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (location_is_directory) {
                pending_viewing_location = location;
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