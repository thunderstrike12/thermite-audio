#pragma once

#include <SDL3/SDL_dialog.h>

#include "engine/core/io.hpp"

namespace tmt {

void open_file_dialog(
    const std::function<void(IO::FileLocation)>& callback, const std::vector<SDL_DialogFileFilter>& filters = { { "Any", "*" } }, const IO::FileLocation& default_location = {}
);

void open_files_dialog(
    const std::function<void(const std::vector<IO::FileLocation>&)>& callback, const std::vector<SDL_DialogFileFilter>& filters = { { "Any", "*" } },
    const IO::FileLocation& default_location = {}
);

void open_folder_dialog(const std::function<void(IO::FileLocation)>& callback, const IO::FileLocation& default_location = {});

void save_file_dialog(
    const std::function<void(IO::FileLocation)>& callback, const std::vector<SDL_DialogFileFilter>& filters = { { "Any", "*" } }, const IO::FileLocation& default_location = {}
);

}  // namespace tmt