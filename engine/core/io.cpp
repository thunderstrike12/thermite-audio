#include "io.hpp"

#include <fstream>
#include "logger.hpp"

namespace tmt {

namespace {

// Function from: https://stackoverflow.com/questions/67144806/c-check-if-path-is-outside-a-given-directory
// Function to check if a file path is below a certain directory/folder in the file hierarchy.
bool directory_contains_path(const std::filesystem::path& directory, const std::filesystem::path& path) {
    const auto& cannon_directory = canonical(directory);
    const auto& cannon_path = canonical(path);

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

/* Absolute path */
std::filesystem::path IO::FileLocation::get_absolute_path() const { return absolute(get_relative_path()); }

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

bool IO::file_exists(const FileLocation& file_location) { return std::filesystem::exists(file_location.get_absolute_path()); }

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

    return {bytes.data(), bytes.size()};
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
        file_stream.close();
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
    if (!exists(path)) {
        Log::error("Failed to make FileLocation from path: path does not exist.");
        return {};
    }

    size_t location_index = 0;
    for (const std::filesystem::path& sub_location : SUB_LOCATIONS) {
        if (directory_contains_path(sub_location, path)) return {static_cast<Location>(location_index), relative(path, absolute(sub_location))};
        ++location_index;
    }

    Log::error("Failed to make FileLocation from path: couldn't create relative path.");
    return {};
}

}  // namespace tmt
