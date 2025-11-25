#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

namespace tmt {

class Log {
   public:
    // Call once at the beginning of the setup

    static void init(const std::string& log_file = "") {
        // This prints: [info] message for the global function
        // the info part is the colored output which can be info, warning,
        // error, etc check
        // https://github.com/gabime/spdlog/wiki/Custom-formatting
        spdlog::set_pattern("%^[%l]%$ %v");

        loggers[Scope::ENGINE] = spdlog::stdout_color_mt("Engine");
        loggers[Scope::GAME] = spdlog::stdout_color_mt("Game");
        loggers[Scope::RENDERER] = spdlog::stdout_color_mt("Renderer");

        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink = nullptr;
        if (!log_file.empty()) {
            file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                log_file, true
            );
            file_sink->set_pattern("[%n] %^[%l]%$ %v");
        }
        for (const auto& logger : loggers) {
            // Pattern with logger name (%n) and level, only the logger level is
            // colored
            logger.second->set_pattern("[%n] %^[%l]%$ %v");

            if (file_sink != nullptr) {
                logger.second->sinks().push_back(file_sink);
            }
        }
    }
    enum class Scope {
        ENGINE,
        RENDERER,
        GAME,
    };

    template <typename... Args>
    static void info(
        Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args
    ) {
        loggers[scope]->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(
        Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args
    ) {
        loggers[scope]->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(
        Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args
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
    static inline std::unordered_map<Scope, std::shared_ptr<spdlog::logger>>
        loggers;
};

}  // namespace tmt
