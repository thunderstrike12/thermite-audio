#include "directory_watcher.hpp"

#include "core/logger.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace tmt {

DirectoryWatcher::DirectoryWatcher(const IO::FileLocation& location, const bool watch_recursively, bool avoid_double_notify, const WatchReason watch_reasons)
    : DirectoryWatcher {absolute(location.get_relative_path()), watch_recursively, avoid_double_notify, watch_reasons} {}

DirectoryWatcher::DirectoryWatcher(const std::filesystem::path& path, const bool watch_recursively, bool avoid_double_notify, const WatchReason watch_reasons)
    : avoid_double_notify {avoid_double_notify} {
    if (!is_directory(path)) {
        Log::error("DirectoryWatcher: Failed to create directory watcher because the given path is not a directory.");

        return;
    }

    if (path.empty() || !exists(path)) {
        Log::error("DirectoryWatcher: Failed to create directory watcher because of an invalid path.");
        return;
    }

    const std::string path_string = path.generic_string();
    watching_handle = FindFirstChangeNotification(path_string.c_str(), watch_recursively, static_cast<DWORD>(watch_reasons));

    if (watching_handle == INVALID_HANDLE_VALUE) {
        watching_handle = nullptr;

        Log::error("DirectoryWatcher: Failed to create directory watcher, error code {}.", GetLastError());
        return;
    }
}

DirectoryWatcher::DirectoryWatcher(DirectoryWatcher&& other) noexcept
    : watching_handle {other.watching_handle}, avoid_double_notify {other.avoid_double_notify}, last_update_time {other.last_update_time} {
    other.watching_handle = nullptr;
    other.last_update_time = INVALID_TIME;
}

DirectoryWatcher& DirectoryWatcher::operator=(DirectoryWatcher&& other) noexcept {
    clear();

    watching_handle = other.watching_handle;
    other.watching_handle = nullptr;

    avoid_double_notify = other.avoid_double_notify;

    last_update_time = other.last_update_time;
    other.last_update_time = INVALID_TIME;

    return *this;
}

bool DirectoryWatcher::check_changes() {
    if (!is_valid()) {
        Log::error("DirectoryWatcher: Instance was not created correctly, watcher is invalid.");

        return false;
    }

    // Wait status will be set to WAIT_OBJECT_0 if the first (and only) object we were waiting on has changed.
    const DWORD wait_status = WaitForSingleObject(watching_handle, 0);
    bool has_changed = (wait_status == WAIT_OBJECT_0);

    const auto now = std::chrono::system_clock::now();
    if (has_changed) {
        if (FindNextChangeNotification(watching_handle) == false) {
            Log::error("DirectoryWatcher: Failed updating watcher, error code {}.", GetLastError());
            clear();
        }
        last_update_time = now;
    } else {
        if (wait_status == WAIT_FAILED) Log::error("DirectoryWatcher: Failed to check directory, error code {}.", GetLastError());
    }

    // If we try to avoid double notifying about changes we should check if there were any changes actually made to the file and then handle timing.
    if (avoid_double_notify && last_update_time != INVALID_TIME) {
        has_changed = (last_update_time + DOUBLE_CHANGE_BUFFER_TIME < now);

        if (has_changed) last_update_time = INVALID_TIME;
    }

    return has_changed;
}

void DirectoryWatcher::clear() {
    if (is_valid() && watching_handle != INVALID_HANDLE_VALUE) FindCloseChangeNotification(watching_handle);

    watching_handle = nullptr;
}

FileWatcher::FileWatcher(const IO::FileLocation& location, bool avoid_double_notify, WatchReason watch_reasons)
    : FileWatcher {absolute(location.get_relative_path()), avoid_double_notify, watch_reasons} {}

FileWatcher::FileWatcher(const std::filesystem::path& path, bool avoid_double_notify, WatchReason watch_reasons) {
    if (is_directory(path)) {
        Log::error("FileWatcher: Failed to create file watcher because the given path is not a file.");

        return;
    }

    if (path.empty() || !exists(path)) {
        Log::error("FileWatcher: Failed to create directory watcher because of an invalid path.");

        return;
    }

    if (!path.has_parent_path()) {
        Log::error("FileWatcher: Failed to create directory watcher because the given path has no parent directory.");

        return;
    }

    file_path = path;
    directory_watcher = DirectoryWatcher {path.parent_path(), false, avoid_double_notify, watch_reasons};
    last_write_time_cache = std::filesystem::last_write_time(path);
}

FileWatcher::FileWatcher(FileWatcher&& other) noexcept
    : file_path {std::move(other.file_path)}, directory_watcher {std::move(other.directory_watcher)}, last_write_time_cache {other.last_write_time_cache} {
    other.last_write_time_cache = {};
}

FileWatcher& FileWatcher::operator=(FileWatcher&& other) noexcept {
    clear();

    file_path = std::move(other.file_path);
    directory_watcher = std::move(other.directory_watcher);
    last_write_time_cache = other.last_write_time_cache;
    other.last_write_time_cache = {};

    return *this;
}

bool FileWatcher::check_changes() {
    if (!is_valid()) {
        Log::error("FileWatcher: Instance was not created correctly, watcher is invalid.");

        return false;
    }

    if (!directory_watcher.check_changes()) return false;

    if (!exists(file_path) || is_directory(file_path)) {
        Log::error("FileWatcher: Failed to check file path does not exist or is a directory.");

        return false;
    }

    const std::filesystem::file_time_type last_write_time = std::filesystem::last_write_time(file_path);
    if (last_write_time <= last_write_time_cache) return false;

    last_write_time_cache = last_write_time;

    return true;
}

void FileWatcher::clear() {
    file_path.clear();
    directory_watcher.clear();
    last_write_time_cache = {};
}

}  // namespace tmt