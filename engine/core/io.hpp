#pragma once

#include <string>
#include <filesystem>

#include "engine/tools/fmt/stl.hpp"
#include "engine/tools/fmt/enum.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

using TimeStamp = std::filesystem::file_time_type;

class IO {
   public:
    enum class Location : uint8_t { PROJECT, ENGINE, EDITOR };

    struct FileLocation {
        IO::Location sub_location;
        std::filesystem::path relative_path;

        bool operator==(const FileLocation& other) const noexcept { return sub_location == other.sub_location && relative_path == other.relative_path; }

        std::filesystem::path get_relative_path() const;
        std::filesystem::path get_absolute_path() const;
    };

    struct FileLocationHash {
        std::size_t operator()(const IO::FileLocation& fl) const noexcept {
            using Underlying = std::underlying_type_t<IO::Location>;

            const size_t h1 = std::hash<Underlying> {}(static_cast<Underlying>(fl.sub_location));

            const size_t h2 = std::hash<std::filesystem::path> {}(fl.relative_path);

            // https://www.reddit.com/r/cpp/comments/1225m8g/why_is_there_no_standard_library_way_to_combine/
            return h1 ^ (h2 + 0x9e3779b9ull + (h1 << 6) + (h1 >> 2));
        }
    };

    // Write arbitrary data, returns success
    static bool write_file(const FileLocation& file_location, const char* data, size_t size);

    // Write text to file, overwrites if file exists. set overwrite = false to append. returns success
    static bool write_text_file(const FileLocation& file_location, const std::string& text, bool overwrite = true);

    // Check if file exists on disk
    static bool file_exists(const FileLocation& file_location);

    // Read file, returns vector containing bytes
    static std::vector<char> read_file(const FileLocation& file_location);

    // Read text file (primarily for json)
    static std::string read_text_file(const FileLocation& file_location);

    static std::string read_or_create_text_file(const FileLocation& file_location, const std::string& default_contents = "");

    [[nodiscard]] static const std::filesystem::path& get_sub_location_path(const IO::Location sub_location) { return SUB_LOCATIONS[static_cast<uint8_t>(sub_location)]; }

    static TimeStamp get_file_last_modified_time(const FileLocation& file_location);

    [[nodiscard]] static FileLocation path_to_file_location(const std::filesystem::path& path);

   private:
    static inline const std::filesystem::path SUB_LOCATIONS[3] {"assets/game", "assets/engine", "assets/editor"};

    static bool stream_open(std::fstream& file_stream, const std::filesystem::path& absolute, std::ios::openmode open_mode);
    static bool create_directories(const std::filesystem::path& absolute);
};

}  // namespace tmt

FMT_LOGGING(tmt::IO::FileLocation, "({}, \"{}\")", obj.sub_location, obj.relative_path);
TMT_OBJECT(tmt::IO::FileLocation, (sub_location, relative_path));