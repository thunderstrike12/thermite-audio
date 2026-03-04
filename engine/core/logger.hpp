#pragma once

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

#include <csignal>

namespace tmt {

class Log {
   public:
    // Call once at the beginning of the setup
    static void init(const std::string& log_file = "") {
        static bool initialized = false;
        assert(initialized == false);
        initialized = true;

        // This prints: [info] message for the global function
        // the info part is the colored output which can be info, warning,
        // error, etc check
        // https://github.com/gabime/spdlog/wiki/Custom-formatting
        loggers[Scope::ENGINE] = spdlog::stdout_color_mt("Engine");
        loggers[Scope::GAME] = spdlog::stdout_color_mt("Game");
        loggers[Scope::RENDERER] = spdlog::stdout_color_mt("Renderer");
        loggers[Scope::EDITOR] = spdlog::stdout_color_mt("Editor");
        loggers[Scope::GLOBAL] = spdlog::stdout_color_mt("Global");
        spdlog::set_level(spdlog::level::debug);

        std::shared_ptr<spdlog::sinks::basic_file_sink_mt> file_sink = nullptr;
        if (!log_file.empty()) {
            file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
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

        // Set signal functions to be called when certain crashes happen, this is to flush the loggers into the log file.
        (void)std::signal(SIGABRT, &on_signal);
        (void)std::signal(SIGFPE, &on_signal);
        (void)std::signal(SIGILL, &on_signal);
        (void)std::signal(SIGINT, &on_signal);
        (void)std::signal(SIGSEGV, &on_signal);
        (void)std::signal(SIGTERM, &on_signal);
    }

    enum class Scope {
        ENGINE,
        RENDERER,
        GAME,
        EDITOR,
        GLOBAL,
    };

    template <typename... Args>
    static void info(Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[scope]->info(fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void debug(Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[scope]->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[scope]->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(Scope scope, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[scope]->error(fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[Scope::GLOBAL]->debug(fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    static void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[Scope::GLOBAL]->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        spdlog::warn(fmt, std::forward<Args>(args)...);
        loggers[Scope::GLOBAL]->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        loggers[Scope::GLOBAL]->error(fmt, std::forward<Args>(args)...);
    }

    static void add_sink(const std::shared_ptr<spdlog::sinks::sink>& sink) {
        for (auto& [scope, logger] : loggers) {
            logger->sinks().push_back(sink);
        }
    }

    static void remove_sink(const std::shared_ptr<spdlog::sinks::sink>& sink) {
        for (auto& [scope, logger] : loggers) {
            auto& sinks = logger->sinks();
            std::erase(sinks, sink);
        }
    }

    static void flush() {
        for (auto& [scope, logger] : loggers) {
            logger->flush();
        }
    }

   private:
    friend class Console;
    static void on_signal(const int signal) {
        error("Flushing logs due to signal: {}", signal);
        flush();
    }

    static inline std::unordered_map<Scope, std::shared_ptr<spdlog::logger>> loggers;
};

}  // namespace tmt
