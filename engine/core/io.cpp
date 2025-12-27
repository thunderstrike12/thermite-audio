#include "io.hpp"

#include <fstream>
#include "logger.hpp"

namespace tmt {

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

}  // namespace tmt
