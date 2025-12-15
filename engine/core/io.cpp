#include "io.hpp"

#include <fstream>
#include "logger.hpp"

namespace tmt {
const char* IO::path_str[3] = {"assets/game/", "assets/engine/", "assets/editor/"};

bool IO::write_file(const FileLocation& file_location, const char* data, size_t size) {
    std::filesystem::path absolute = get_absolute_path(file_location.sub_location, file_location.relative_path);

    if (!create_directories(absolute)) return false;

    std::fstream output_file;
    if (!stream_open(output_file, absolute, std::ios::out | std::ios::binary)) return false;

    try {
        output_file.write(data, size);
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception while writing file: {}\n reason: {}", absolute.string(), exception.what());
        output_file.close();
        return false;
    }

    output_file.close();

    return true;
}

bool IO::write_text_file(const FileLocation& file_location, const std::string& text, bool overwrite) {
    std::filesystem::path absolute = get_absolute_path(file_location.sub_location, file_location.relative_path);

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
    std::filesystem::path absolute = get_absolute_path(file_location.sub_location, file_location.relative_path);

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
        std::ifstream::pos_type filesize = input_file.tellg();
        input_file.seekg(0, std::ios::beg);
        data.resize(filesize);

        input_file.read(data.data(), filesize);
    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception when reading file: {}\nreason: {}", absolute.string(), exception.what());
    }

    return data;
}

std::string IO::read_text_file(const FileLocation& file_location) {
    std::vector<char> bytes = read_file(file_location);
    if (bytes.empty()) {
        return {};
    }

    return std::string(bytes.data(), bytes.size());
}

std::string IO::read_or_create_text_file(const FileLocation& file_location, const std::string& default_contents) {
    std::filesystem::path absolute = get_absolute_path(file_location.sub_location, file_location.relative_path);
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

std::filesystem::path IO::get_absolute_path(Location sub_cat, const std::filesystem::path& relative_path) {
    auto sub_path = std::filesystem::path(path_str[static_cast<uint8_t>(sub_cat)]);

    std::filesystem::path relative = relative_path.lexically_normal();
    if (relative_path.has_root_path()) {
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "Possible incorrect path passed: {}\nTrying to fix path..", relative_path.string());

        relative = relative_path.relative_path();
    }
    return sub_path / relative;
}

bool IO::stream_open(std::fstream& file_stream, const std::filesystem::path& absolute, std::ios::openmode open_mode) {
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
        // create directiories if they didn't exist yet
        bool created = std::filesystem::create_directories(absolute.parent_path());

        if (created) tmt::Log::warn(tmt::Log::Scope::ENGINE, "Created directories while writing: {}", absolute.parent_path().string());

    } catch (const std::exception& exception) {
        tmt::Log::error(tmt::Log::Scope::ENGINE, "Exception when creating directories: {}\nreason: {}", absolute.parent_path().string(), exception.what());
        return false;
    }
    return true;
}

}  // namespace tmt
