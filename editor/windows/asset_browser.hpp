#pragma once

#include "editor/core/window.hpp"
#include "engine/core/io.hpp"
#include "engine/tools/directory_watcher.hpp"

#include <stack>
#include <imgui.h>

namespace tmt {

class AssetBrowser : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_FOLDER " Asset Browser"; }
    constexpr int get_window_flags() const override { return ImGuiWindowFlags_MenuBar; }
    constexpr bool default_open() const override { return true; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override;
    void on_editor_end() override {}

   private:
    struct Directory {
        IO::FileLocation location;
        std::vector<Directory> sub_directories {};
    };

    struct Bookmark {
        Directory directory;
        DirectoryWatcher location_watcher;
    };

    std::optional<IO::FileLocation> pending_viewing_location {viewing_location};
    IO::FileLocation viewing_location {};
    DirectoryWatcher viewing_location_watcher {};
    std::vector<IO::FileLocation> viewing_locations;
    std::vector<IO::FileLocation> selected_locations;

    std::stack<IO::FileLocation> undo_viewing_locations;
    std::stack<IO::FileLocation> redo_viewing_locations;

    std::vector<Bookmark> default_bookmarks;
    std::vector<Bookmark> bookmarks;

    static void update_bookmark_vector(std::vector<Bookmark>& bookmarks);
    static void recurse_parse_directory(Directory& directory);

    [[nodiscard]] bool location_is_bookmarked(const IO::FileLocation& location);

    void update_location_history();
    void update_viewing_locations();

    void move_location_stacks(std::stack<IO::FileLocation>& from, std::stack<IO::FileLocation>& to);
    ImGuiID recurse_display_bookmark_dirs(const Directory& directory);

    void location_context_menu(const IO::FileLocation& location, bool allow_bookmark);
    void viewing_context_menu() const;

    void display_directory_bar();
    void display_bookmarks();
    void display_viewing_location();
};
}  // namespace tmt