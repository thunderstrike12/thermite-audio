#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

namespace tmt {

class Log {
   public:
    // Call once at the beginning of the setup
    static void init() {
        // This prints: [info] message for the global function
        // the info part is the colored output which can be info, warning,
        // error, etc check
        // https://github.com/gabime/spdlog/wiki/Custom-formatting
        spdlog::set_pattern("%^[%l]%$ %v");
        loggers[LoggerScope::ENGINE] = spdlog::stdout_color_mt("Engine");
        loggers[LoggerScope::GAME] = spdlog::stdout_color_mt("Game");
        loggers[LoggerScope::RENDERER] = spdlog::stdout_color_mt("Renderer");

        // Pattern with logger name (%n) and level
        for (const auto& logger : loggers) {
            logger.second->set_pattern("%^[%n] [%l]%$ %v");
        }
    }
    enum class LoggerScope {
        ENGINE,
        RENDERER,
        GAME,
    };

    template <typename... Args>
    static void info(
        LoggerScope scope, spdlog::format_string_t<Args...> fmt, Args&&... args
    ) {
        loggers[scope]->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(
        LoggerScope scope, spdlog::format_string_t<Args...> fmt, Args&&... args
    ) {
        loggers[scope]->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(
        LoggerScope scope, spdlog::format_string_t<Args...> fmt, Args&&... args
    ) {
        loggers[scope]->error(fmt, std::forward<Args>(args)...);
    }

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

   private:
    static inline std::unordered_map<
        LoggerScope, std::shared_ptr<spdlog::logger>>
        loggers;
};

}  // namespace tmt
