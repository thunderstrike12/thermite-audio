#pragma once
#include <memory>
#include <stdexcept>
#include <vector>

#include "time_step.hpp"
#include "system.hpp"

#include "events/game.hpp"

namespace tmt {

class Ecs : public OnStart,
            public OnUpdate,
            public OnFixedUpdate,
            public OnEnd {
   public:
    Ecs() = default;
    ~Ecs() = default;

    template <typename T, typename... Args>
    T& register_system(Args&&... args) {
        static_assert(
            std::is_base_of_v<ISystem, T>, "T must inherit from ISystem"
        );

        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;

        systems.push_back(std::move(system));
        return ref;
    }

    template <typename T>
    T& get_system() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T>
    T* try_get_system() {
        for (auto& system : systems) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
        return nullptr;
    }

   private:
    std::vector<std::unique_ptr<ISystem>> systems;

    void on_start() override;

    void on_update(const FrameData& time) override;

    void on_fixed_update(const FrameData& time) override;

    void on_end() override;
};

}  // namespace tmt