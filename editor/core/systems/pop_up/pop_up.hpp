#pragma once
#include <spdlog/fmt/fmt.h>

#include <string>
#include <vector>
#include "engine/tools/types/severity.hpp"

namespace tmt {

/* Mixins implementations */
namespace detail {

template <typename T>
class Title {
   public:
    template <typename... Args>
    T& title(fmt::format_string<Args...> fmt, Args&&... args) {
        title_text = fmt::format(fmt, std::forward<Args>(args)...);
        return static_cast<T&>(*this);
    }

   protected:
    std::string title_text = "";
};

template <typename T>
class Message {
   public:
    template <typename... Args>
    T& message(fmt::format_string<Args...> fmt, Args&&... args) {
        message_text = fmt::format(fmt, std::forward<Args>(args)...);
        return static_cast<T&>(*this);
    }

   protected:
    std::string message_text = "";
};

template <typename T>
class SeverityMixin {
   public:
    T& severity(Severity severity) {
        severity_level = severity;
        return static_cast<T&>(*this);
    }

   protected:
    Severity severity_level = Severity::INFO;
};

}  // namespace detail

/* =================== Pop-Up =================== */
class PopUp;

/* Singleton */
class PopUpManager {
   public:
    static PopUp& create_pop_up() {
        queue.emplace_back();
        return queue.back();
    }

    static void add_pop_up(const PopUp& pop_up) { queue.push_back(pop_up); }

    static void update();

   private:
    inline static std::vector<PopUp> queue;
};

class PopUp : public detail::Title<PopUp>, public detail::Message<PopUp>, public detail::SeverityMixin<PopUp> {
   public:
    /* Should only be used by the manager */
    PopUp() = default;

    static PopUp& create() { return PopUpManager::create_pop_up(); }

    PopUp& add_button(const std::string& label, bool closes_pop_up = true, std::function<void()> callback = nullptr) {
        buttons.push_back({ closes_pop_up, label, callback });
        return *this;
    }

    PopUp& show_close_button(bool show = true) {
        close_button = show;
        return *this;
    }

    /* Only needed if you store a pop-up and want to reuse it */
    void add() const { PopUpManager::add_pop_up(*this); }

   private:
    struct Button {
        bool closes_pop_up = false;
        std::string label = "";
        std::function<void()> callback;

        Button() = default;
        Button(bool closes, const std::string& label) : closes_pop_up(closes), label(label) {}
        Button(bool closes, const std::string& label, std::function<void()> callback) : closes_pop_up(closes), label(label), callback(std::move(callback)) {}
    };

   private:
    friend class PopUpManager;

    bool close_button = true;

    std::vector<Button> buttons;
};

/* =================== Notification =================== */
class Notification;

/* Singleton */
class NotificationManager {
   public:
    static Notification& create_notification() {
        queue.emplace_back();
        return queue.back();
    }

    static void add_notification(const Notification& notification) { queue.push_back(notification); }

    static void update(const float delta_time);

   private:
    inline static std::vector<Notification> queue;
};

class Notification : public detail::Title<Notification>, public detail::Message<Notification>, public detail::SeverityMixin<Notification> {
   public:
    /* Should only be used by the manager */
    Notification() = default;

    static Notification& create() { return NotificationManager::create_notification(); }

    Notification& duration(float seconds) {
        duration_seconds = seconds;
        return *this;
    }

    void add() const { NotificationManager::add_notification(*this); }

   private:
    friend class NotificationManager;

    /* In Seconds*/
    float duration_seconds = 3.0f;
};

}  // namespace tmt