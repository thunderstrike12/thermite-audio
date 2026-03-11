#include "io.hpp"

#include <fstream>
#include "logger.hpp"

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace tmt {

std::filesystem::path IO::sub_locations[3] {};
const char* project_relative_dir;

extern "C" const char* TMT_PROJECT_RELATIVE_ASSETS_DIR;

namespace {

// Function from: https://stackoverflow.com/questions/67144806/c-check-if-path-is-outside-a-given-directory
// Function to check if a file path is below a certain directory/folder in the file hierarchy.
bool directory_contains_path(const std::filesystem::path& directory, const std::filesystem::path& path) {
    const std::filesystem::path& cannon_directory = canonical(directory);
    const std::filesystem::path& cannon_path = (exists(path) ? canonical(path) : path);

    auto directory_iterator = cannon_directory.begin();
    for (const auto& sub_path : cannon_path) {
        if (directory_iterator == cannon_directory.end()) break;

        if (*directory_iterator != sub_path) return false;
        ++directory_iterator;
    }

    return true;
}

}  // namespace

/* Relative to working directory */
std::filesystem::path IO::FileLocation::get_relative_path() const {
    const std::filesystem::path& sub_path = get_sub_location_path(sub_location);
    // The check avoids returning a path ending with a directory separator which can be annoying in some cases.
    if (relative_path.empty()) return sub_path;

    return sub_path / relative_path;
}

void IO::init_mounts() {
    // 0 : PROJECT
    // 1 : ENGINE
    // 2 : EDITOR

    auto exe_path = get_exec_path();
    auto pair = find_root(exe_path);

    bool packaged_mode = pair.first;
    auto& root_path = pair.second;

    if (packaged_mode) {
        sub_locations[0] = root_path / "assets/";
        sub_locations[1] = root_path / "assets/engine";
        sub_locations[2] = root_path / "assets/editor";
    } else {
        sub_locations[0] = root_path / TMT_PROJECT_RELATIVE_ASSETS_DIR;
        sub_locations[1] = root_path / "engine/assets";
        sub_locations[2] = root_path / "editor/assets";
    }
}

/* Get the exe file path (win specific) */
std::filesystem::path IO::get_exec_path() {
    std::wstring buf;
    buf.resize(32768);
    DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));

    if (len == 0 || len >= buf.size()) throw std::runtime_error("failed to get module file name!");
    buf.resize(len);
    return std::filesystem::path(buf);
}

std::pair<bool, std::filesystem::path> IO::find_root(const std::filesystem::path& exe_dir) {
    auto has_marker = [](const std::filesystem::path& p) { return (std::filesystem::exists(p / "engine/assets") && std::filesystem::exists(p / "editor/assets")); };

    // iterates backward through parent paths to find signature file structure
    for (auto p = exe_dir; !p.empty(); p = p.parent_path())
        if (has_marker(p)) return std::make_pair(false, p);

    // not found, we might be in packaged mode
    if (std::filesystem::exists(exe_dir / "assets/engine")) return std::make_pair(true, exe_dir);

    // Root not found - throw an error as this is an unrecoverable state
    throw std::runtime_error("Could not find root directory - neither development nor packaged mode detected!");
}

/* Absolute path */
std::filesystem::path IO::FileLocation::get_absolute_path() const {
    return absolute(get_relative_path());
}

bool IO::write_file(const FileLocation& file_location, const char* data, const size_t size) {
    const std::filesystem::path absolute = file_location.get_absolute_path();

    if (!create_directories(absolute)) return false;

    std::fstream output_file;
    if (!stream_open(output_file, absolute, std::ios::out | std::ios::binary)) return false;

    try {
        output_file.write(data, static_cast<std::streamsize>(size));
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception while writing file: {}\n reason: {}", absolute.string(), exception.what());
        output_file.close();
        return false;
    }

    output_file.close();

    return true;
}

bool IO::write_text_file(const FileLocation& file_location, const std::string& text, bool overwrite) {
    const std::filesystem::path& absolute = file_location.get_absolute_path();

    if (!create_directories(absolute)) return false;

    std::fstream output_file;
    if (!stream_open(output_file, absolute, std::ios::out | (overwrite ? std::ios::trunc : std::ios::app))) return false;

    try {
        output_file << text;
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception while writing file: {}\n reason: {}", absolute.string(), exception.what());
        output_file.close();
        return false;
    }

    output_file.close();

    return true;
}

bool IO::file_exists(const FileLocation& file_location) {
    return std::filesystem::exists(file_location.get_absolute_path());
}

std::vector<char> IO::read_file(const FileLocation& file_location) {
    const std::filesystem::path& absolute = file_location.get_absolute_path();

    std::vector<char> data;

    if (!std::filesystem::exists(absolute)) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Could not find file at: {}", absolute.string());
        return data;
    }

    std::fstream input_file;
    if (!stream_open(input_file, absolute, std::ios::in | std::ios::binary)) return data;

    try {
        // find out size of input_file contents
        input_file.seekg(0, std::ios::end);
        const std::ifstream::pos_type file_size = input_file.tellg();
        input_file.seekg(0, std::ios::beg);
        data.resize(file_size);

        input_file.read(data.data(), file_size);
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception when reading file: {}\nreason: {}", absolute.string(), exception.what());
    }

    return data;
}

std::string IO::read_text_file(const FileLocation& file_location) {
    const std::vector<char> bytes = read_file(file_location);
    if (bytes.empty()) {
        return {};
    }

    return { bytes.data(), bytes.size() };
}

std::string IO::read_or_create_text_file(const FileLocation& file_location, const std::string& default_contents) {
    const std::filesystem::path& absolute = file_location.get_absolute_path();
    if (!std::filesystem::exists(absolute)) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "File not found at: {}\nCreating file with default contents.", absolute.string());
        if (!write_text_file(file_location, default_contents, true)) {
            tmt::Log::error(tmt::Log::Scope::ENGINE, "Failed to create file at: {}", absolute.string());
            return {};
        }
        return default_contents;
    }
    return read_text_file(file_location);
}

bool IO::stream_open(std::fstream& file_stream, const std::filesystem::path& absolute, std::ios::openmode open_mode) {
    if (std::filesystem::is_directory(absolute)) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Tried to open a directory as a file: {}", absolute.string());
        return false;
    }
    file_stream.exceptions(std::ios::badbit | std::ios::failbit);
    try {
        file_stream.open(absolute, open_mode);
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception when opening file: {}\nreason: {}", absolute.string(), exception.what());
        return false;
    }
    return true;
}

bool IO::create_directories(const std::filesystem::path& absolute) {
    try {
        // create directories if they didn't exist yet
        const bool created = std::filesystem::create_directories(absolute.parent_path());

        if (created) tmt::Log::warn(tmt::Log::Scope::ENGINE, "Created directories while writing: {}", absolute.parent_path().string());

    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception when creating directories: {}\nreason: {}", absolute.parent_path().string(), exception.what());
        return false;
    }
    return true;
}

TimeStamp IO::get_file_last_modified_time(const FileLocation& file_location) {
    const std::filesystem::path& absolute = file_location.get_absolute_path();
    try {
        return std::filesystem::last_write_time(absolute);
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception when getting last modified time for file: {}\nreason: {}", absolute.string(), exception.what());
        return TimeStamp::min();
    }
}

IO::FileLocation IO::path_to_file_location(const std::filesystem::path& path) {
    // Try to find an IO::FileLocation for the given path, will return a correct location if the path is relative to any asset directory.
    size_t location_index = 0;
    for (const std::filesystem::path& sub_location : sub_locations) {
        if (directory_contains_path(sub_location, path)) return { static_cast<Location>(location_index), relative(path, absolute(sub_location)) };
        ++location_index;
    }

    // As a fallback for file paths that aren't relative to any asset directory, we make it relative to the project assets using a lot of ../../../ paths.
    FileLocation fallback_location { Location::PROJECT, "" };
    fallback_location.relative_path = std::filesystem::relative(path, fallback_location.get_absolute_path());
    return fallback_location;
}

}  // namespace tmt
