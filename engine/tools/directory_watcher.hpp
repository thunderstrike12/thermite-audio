#pragma once

#include "engine/core/io.hpp"

#include <chrono>

namespace tmt {

enum class WatchReason : uint8_t {
    FILE_NAME_CHANGE = 0b1 << 0,
    DIR_NAME_CHANGE = 0b1 << 1,
    ATTRIBUTE_CHANGE = 0b1 << 2,
    SIZE_CHANGE = 0b1 << 3,
    LAST_WRITE_CHANGE = 0b1 << 4,
    LAST_ACCESS_CHANGE = 0b1 << 5,
    CREATION = 0b1 << 6,
    SECURITY_CHANGE = 0b1 << 7,

    VISUAL = FILE_NAME_CHANGE | DIR_NAME_CHANGE | CREATION,
    ANY = FILE_NAME_CHANGE | DIR_NAME_CHANGE | ATTRIBUTE_CHANGE | SIZE_CHANGE | LAST_WRITE_CHANGE | LAST_ACCESS_CHANGE | CREATION | SECURITY_CHANGE
};
using namespace magic_enum::bitwise_operators;

// A class to easily and efficiently watch a certain directory for changes.
class DirectoryWatcher {
    // Time to ignore incoming change notices, this is to avoid double notifying about changes (some programs e.g. vengi, when they save a file will write an empty file to disk before write
    // the new contents to the next frame).
    static constexpr std::chrono::milliseconds DOUBLE_CHANGE_BUFFER_TIME {34};
    // Invalid time, used to clear the last_update_time after preemptively notifying about a changed file when avoiding double notifies.
    static constexpr std::chrono::time_point<std::chrono::system_clock> INVALID_TIME {};

   public:
    // Constructs an invalid DirectoryWatcher instance, can be set to a valid instance using the move assignment operator.
    DirectoryWatcher() = default;

    /// @param location: The location to watch for changes, must be a directory.
    /// @param watch_recursively: Also notifies about folders recursively in the folder structure.
    /// @param avoid_double_notify: Avoid multiple notifications directly after each other (this sometimes happens when certain applications e.g. vengi, save a file).
    /// @param watch_reasons: Flags for changed events to check for using "check_changes()".
    DirectoryWatcher(const IO::FileLocation& location, bool watch_recursively = true, bool avoid_double_notify = false, WatchReason watch_reasons = WatchReason::ANY);

    /// @param path: The path to watch for changes, must be a directory.
    /// @param watch_recursively: Also notifies about folders recursively in the folder structure.
    /// @param avoid_double_notify: Avoid multiple notifications directly after each other (this sometimes happens when certain applications e.g. vengi, save a file).
    /// @param watch_reasons: Flags for changed events to check for using "check_changes()".
    DirectoryWatcher(const std::filesystem::path& path, bool watch_recursively = true, bool avoid_double_notify = false, WatchReason watch_reasons = WatchReason::ANY);

    // Move constructor, invalidates the other DirectoryWatcher.
    DirectoryWatcher(DirectoryWatcher&& other) noexcept;
    // Move assignment operator, invalidates the other DirectoryWatcher.
    DirectoryWatcher& operator=(DirectoryWatcher&& other) noexcept;

    // Delete the copy constructors since we don't want to accidentally copy these (usually not intended).
    DirectoryWatcher(DirectoryWatcher&) = delete;
    DirectoryWatcher(const DirectoryWatcher&) = delete;
    DirectoryWatcher(const DirectoryWatcher&&) = delete;

    [[nodiscard]] bool is_valid() const { return watching_handle != nullptr; }
    // Check for changes efficiently (can be called every frame) returns true if a change has happened in the directory since the last check, false otherwise.
    [[nodiscard]] bool check_changes();
    // Invalidate the DirectoryWatcher.
    void clear();

   private:
    // Handle for the change event watcher from windows.h.
    void* watching_handle {nullptr};
    bool avoid_double_notify {false};

    std::chrono::time_point<std::chrono::system_clock> last_update_time {INVALID_TIME};
};

// A class to easily and efficiently watch a certain file for changes.
class FileWatcher {
   public:
    // Constructs an invalid FileWatcher instance, can be set to a valid instance using the move assignment operator.
    FileWatcher() = default;

    /// @param location: The location to watch for changes, must be a file.
    /// @param avoid_double_notify: Avoid multiple notifications directly after each other (this sometimes happens when certain applications e.g. vengi, save a file).
    /// @param watch_reasons: Flags for changed events to check for using "check_changes()".
    FileWatcher(const IO::FileLocation& location, bool avoid_double_notify = true, WatchReason watch_reasons = WatchReason::LAST_WRITE_CHANGE);

    /// @param path: The path to watch for changes, must be a file.
    /// @param avoid_double_notify: Avoid multiple notifications directly after each other (this sometimes happens when certain applications e.g. vengi, save a file).
    /// @param watch_reasons: Flags for changed events to check for using "check_changes()".
    FileWatcher(const std::filesystem::path& path, bool avoid_double_notify = true, WatchReason watch_reasons = WatchReason::LAST_WRITE_CHANGE);

    // Move constructor, invalidates the other FileWatcher.
    FileWatcher(FileWatcher&& other) noexcept;
    // Move assignment operator, invalidates the other FileWatcher.
    FileWatcher& operator=(FileWatcher&& other) noexcept;

    // Delete the copy constructors since we don't want to accidentally copy these (usually not intended).
    FileWatcher(FileWatcher&) = delete;
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher(const FileWatcher&&) = delete;

    // Get the cached value of the last time the file was written to.
    [[nodiscard]] std::filesystem::file_time_type get_last_write_time() const { return last_write_time_cache; }

    [[nodiscard]] bool is_valid() const { return !file_path.empty() && directory_watcher.is_valid(); }
    // Check for changes efficiently (can be called every frame) returns true if the file has changed since the last check, false otherwise.
    [[nodiscard]] bool check_changes();
    // Invalidate the FileWatcher.
    void clear();

   private:
    std::filesystem::path file_path;
    // Directory watcher to check for changes in the directory of the file, if it's check returns true we check if the file itself changed..
    DirectoryWatcher directory_watcher;

    std::filesystem::file_time_type last_write_time_cache {};
};

}  // namespace tmt