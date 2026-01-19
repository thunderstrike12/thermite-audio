#include "file_dialog.hpp"

#include <SDL3/SDL_dialog.h>

#include "engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/window.hpp"

namespace tmt {

namespace {

std::function<void(IO::FileLocation)> single_file_dialog {};
void single_file_dialog_function(void*, const char* const* file_list, int) {
    if (file_list == nullptr) {
        Log::error("calling file_dialog callback failed.");
        return;
    }

    if (file_list[0] == nullptr) return;

    single_file_dialog(IO::path_to_file_location(file_list[0]));
}

std::function<void(const std::vector<IO::FileLocation>&)> multi_file_dialog {};
void multi_file_dialog_function(void*, const char* const* file_list, int) {
    if (file_list == nullptr) {
        Log::error("calling file_dialog callback failed.");
        return;
    }

    std::vector<IO::FileLocation> file_locations;

    size_t i = 0;
    const char* next_path = file_list[i];
    while (next_path != nullptr) {
        file_locations.push_back(IO::path_to_file_location(next_path));
        next_path = file_list[++i];
    }

    multi_file_dialog(file_locations);
}

}  // namespace

void open_file_dialog(const std::function<void(IO::FileLocation)>& callback, const std::vector<SDL_DialogFileFilter>& filters, const IO::FileLocation& default_location) {
    single_file_dialog = callback;

    // Using path.string() instead of path.generic_string() because SDL3 will complain otherwise, it'll still work, but it'll *also* give an error.
    const std::string& default_location_string = default_location.get_absolute_path().string();
    SDL_ShowOpenFileDialog(&single_file_dialog_function, nullptr, engine.window.window, filters.data(), static_cast<int>(filters.size()), default_location_string.c_str(), false);
}

void open_files_dialog(const std::function<void(const std::vector<IO::FileLocation>&)>& callback, const std::vector<SDL_DialogFileFilter>& filters, const IO::FileLocation& default_location) {
    multi_file_dialog = callback;

    // Using path.string() instead of path.generic_string() because SDL3 will complain otherwise, it'll still work, but it'll *also* give an error.
    const std::string& default_location_string = default_location.get_absolute_path().string();
    SDL_ShowOpenFileDialog(&multi_file_dialog_function, nullptr, engine.window.window, filters.data(), static_cast<int>(filters.size()), default_location_string.c_str(), true);
}

void open_folder_dialog(const std::function<void(IO::FileLocation)>& callback, const IO::FileLocation& default_location) {
    single_file_dialog = callback;

    // Using path.string() instead of path.generic_string() because SDL3 will complain otherwise, it'll still work, but it'll *also* give an error.
    const std::string& default_location_string = default_location.get_absolute_path().string();
    SDL_ShowOpenFolderDialog(&single_file_dialog_function, nullptr, engine.window.window, default_location_string.c_str(), false);
}

void save_file_dialog(const std::function<void(IO::FileLocation)>& callback, const std::vector<SDL_DialogFileFilter>& filters, const IO::FileLocation& default_location) {
    single_file_dialog = callback;

    // Using path.string() instead of path.generic_string() because SDL3 will complain otherwise, it'll still work, but it'll *also* give an error.
    const std::string& default_location_string = default_location.get_absolute_path().string();
    SDL_ShowSaveFileDialog(&multi_file_dialog_function, nullptr, engine.window.window, filters.data(), static_cast<int>(filters.size()), default_location_string.c_str());
}

}  // namespace tmt