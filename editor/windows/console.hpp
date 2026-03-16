#pragma once
#include "editor/core/window.hpp"
#include "spdlog/sinks/base_sink.h"

#include <imgui_console.h>

namespace tmt {

// This sink is specific to the console window, I was unsure where to place it.
template <typename Mutex>
class ConsoleSink : public spdlog::sinks::base_sink<Mutex> {
   public:
    explicit ConsoleSink(csys::System& system) : system(system) {}

   protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        std::string message = fmt::to_string(formatted);
        if (!message.empty() && message.back() == '\n') message.pop_back();

        csys::ItemType type {};
        switch (msg.level) {
            case spdlog::level::debug:
                type = csys::ItemType::LOG;
                break;
            case spdlog::level::info:
                type = csys::ItemType::INFO;
                break;
            case spdlog::level::warn:
                type = csys::ItemType::WARNING;
                break;
            case spdlog::level::err:
            case spdlog::level::critical:
                type = csys::ItemType::ERROR;
                break;
            default:
                type = csys::ItemType::LOG;
                break;
        }

        system.Log(type) << message;
    }

    void flush_() override {}

   private:
    csys::System& system;
};

using ConsoleSink_st = ConsoleSink<spdlog::details::null_mutex>;

class Console : public IWindow<> {
   public:
    void on_inspect() override;

    constexpr std::string get_title() const override { return ICON_MS_TERMINAL " Console"; }

    constexpr int get_window_flags() const override { return ImGuiWindowFlags_MenuBar; };

    constexpr bool default_open() const override { return true; }

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;

    template <typename Func, typename... Args>
    void register_command(const std::string& name, const std::string& description, Func&& callback, Args&&... args) {
        console.System().RegisterCommand(name, description, std::forward<Func>(callback), std::forward<Args>(args)...);
    }

    template <typename T>
    void register_variable(const std::string& name, T& variable) {
        console.System().RegisterVariable(name, variable);
    }
    template <typename T, typename... Types>
    void register_variable(const std::string& name, T& variable, void (*setter)(T&, Types...)) {
        console.System().RegisterVariable(name, variable, setter);
    }
    void register_script(const std::string& name, const std::string& filepath) { console.System().RegisterScript(name, filepath); }

   private:
    void register_commands();
    void register_variables();
    void register_scripts();

    ImGuiConsole console;
    std::shared_ptr<ConsoleSink_st> sink;
};

}  // namespace tmt
