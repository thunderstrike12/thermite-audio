/*
Made by Sven van Huessen, https://github.com/Sven-vh/event-system
Feel free to use this anywhere.
*/
#pragma once
#include <vector>
#include <algorithm>

namespace tmt {

class EventBase {
   public:
    virtual ~EventBase() = default;

    bool handled = false;
};

template <typename T, typename E = void>
class EventListenerBase {
   public:
    EventListenerBase() { listeners.push_back(static_cast<T*>(this)); }

    virtual ~EventListenerBase() {
        listeners.erase(
            std::remove(
                listeners.begin(), listeners.end(), static_cast<T*>(this)
            ),
            listeners.end()
        );
    }

    virtual void on_event(E& event) = 0;
    virtual void on_event(const E& event) = 0;

    static void dispatch(E& event) {
        sort_listeners();
        for (auto listener : listeners) {
            listener->on_event(event);
            if constexpr (std::is_base_of_v<EventBase, E>) {
                if (break_on_handled && event.handled) break;
            }
        }
    }

    static void dispatch(const E& event) {
        sort_listeners();
        for (auto listener : listeners) {
            listener->on_event(event);
            if constexpr (std::is_base_of_v<EventBase, E>) {
                if (break_on_handled && event.handled) break;
            }
        }
    }

   protected:
    int priority = 0;
    inline static bool break_on_handled = true;

   private:
    static void sort_listeners() {
        std::sort(listeners.begin(), listeners.end(), [](T* a, T* b) {
            return a->priority > b->priority;
        });
    }

    inline static std::vector<T*> listeners;
};

/* Void Specialization */
template <typename T>
class EventListenerBase<T, void> {
   public:
    EventListenerBase() { listeners.push_back(static_cast<T*>(this)); }

    virtual ~EventListenerBase() {
        listeners.erase(
            std::remove(
                listeners.begin(), listeners.end(), static_cast<T*>(this)
            ),
            listeners.end()
        );
    }

    virtual void on_event() = 0;

    // Static dispatch for void events
    static void dispatch() {
        sort_listeners();
        for (auto listener : listeners) {
            listener->on_event();
        }
    }

   protected:
    int priority = 0;

   private:
    static void sort_listeners() {
        std::sort(listeners.begin(), listeners.end(), [](T* a, T* b) {
            return a->priority > b->priority;
        });
    }

    inline static std::vector<T*> listeners;
};

}  // namespace tmt