#pragma once
#include "spdlog/spdlog.h"

namespace tmt {

class Log {
    Log() {
        // This prints: [info] message
        // the info part is the colored output which can be info, warning,
        // error, etc check
        // https://github.com/gabime/spdlog/wiki/Custom-formatting
        spdlog::set_pattern("%^[%l]%$ %v");
    }

   public:
    template <typename... Args>
    static void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }
};

}  // namespace tmt
