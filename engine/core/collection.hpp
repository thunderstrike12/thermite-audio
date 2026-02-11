#pragma once
#include <vector>
#include <memory>
#include <stdexcept>

namespace tmt {

template <typename CollectionType>
class Collection {
   public:
    /* Systems */
    template <typename T, typename... Args>
    requires std::is_base_of_v<CollectionType, T>
    T& add(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;

        if (try_get<T>() != nullptr) {
            throw std::runtime_error("System of this type already exists");
        }

        systems.push_back(std::move(system));
        return ref;
    }

    template <typename T>
    requires std::is_base_of_v<CollectionType, T>
    T& get() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T>
    requires std::is_base_of_v<CollectionType, T>
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
    auto begin() const { return systems.begin(); }
    auto end() { return systems.end(); }
    auto end() const { return systems.end(); }

    size_t size() const { return systems.size(); }

   protected:
    std::vector<std::unique_ptr<CollectionType>> systems;
};

}  // namespace tmt