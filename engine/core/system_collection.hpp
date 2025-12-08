#pragma once
#include <vector>
#include <memory>
#include <stdexcept>

namespace tmt {
template <typename SystemType>
class SystemCollection {
   public:
    /* Systems */
    template <typename T, typename... Args>
        requires std::is_base_of_v<SystemType, T>
    T& add(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;

        systems.push_back(std::move(system));
        return ref;
    }

    template <typename T>
        requires std::is_base_of_v<SystemType, T>
    T& get() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T>
        requires std::is_base_of_v<SystemType, T>
    T* try_get() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    /* Iterators */
    auto begin() { return systems.begin(); }
    auto end() { return systems.end(); }

   private:
    std::vector<std::unique_ptr<SystemType>> systems;
};
}  // namespace tmt