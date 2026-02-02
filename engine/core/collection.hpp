#pragma once
#include <vector>
#include <memory>
#include <stdexcept>

namespace tmt {

template <typename T, typename... Types>
struct is_base_of_any : std::disjunction<std::is_base_of<Types, T>...> {};

template <typename... CollectionTypes>
class Collection {
    constexpr static bool SINGLE_TYPE = (sizeof...(CollectionTypes) == 1);
    using SingleType = typename std::tuple_element<0, std::tuple<CollectionTypes...>>::type;

   public:
    /* Systems */
    template <typename T, typename... Args>
        requires is_base_of_any<T, CollectionTypes...>::value
    T& add(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;

        if (try_get<T>() != nullptr) {
            throw std::runtime_error("System of this type already exists");
        }

        // systems.push_back(std::move(system));
        (try_add_system<T, CollectionTypes>(std::move(system)), ...);
        return ref;
    }

    template <typename T>
        requires is_base_of_any<T, CollectionTypes...>::value && SINGLE_TYPE
    T& get() {
        for (auto& system : get_systems<SingleType>()) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T, typename CollectionType>
        requires is_base_of_any<T, CollectionTypes...>::value
    T& get() {
        for (auto& system : get_systems<CollectionType>()) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return *casted;
            }
        }
        throw std::runtime_error("System not found");
    }

    template <typename T>
        requires is_base_of_any<T, CollectionTypes...>::value && SINGLE_TYPE
    T* try_get() {
        for (auto& system : get_systems<SingleType>()) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    template <typename T, typename CollectionType>
        requires is_base_of_any<T, CollectionTypes...>::value
    T* try_get() {
        for (auto& system : get_systems<CollectionType>()) {
            if (T* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
    }

    /* Iterators */
    auto begin()
        requires SINGLE_TYPE
    {
        return get_systems_single().begin();
    }

    auto begin() const
        requires SINGLE_TYPE
    {
        return get_systems_single().begin();
    }

    auto end()
        requires SINGLE_TYPE
    {
        return get_systems_single().end();
    }

    auto end() const
        requires SINGLE_TYPE
    {
        return get_systems_single().end();
    }

    size_t size() const
        requires SINGLE_TYPE
    {
        return get_systems_single().size();
    }

   protected:
    // std::vector<std::unique_ptr<CollectionType>> systems;

    template <typename CollectionType>
    std::vector<std::unique_ptr<CollectionType>>& get_systems() const {
        static std::vector<std::unique_ptr<CollectionType>> typed_systems;
        return typed_systems;
    }

   private:
    auto& get_systems_single()
        requires(sizeof...(CollectionTypes) == 1)
    {
        // Extract the single type from the pack
        return get_systems<SingleType>();
    }

    const auto& get_systems_single() const
        requires(sizeof...(CollectionTypes) == 1)
    {
        // Extract the single type from the pack
        return get_systems<SingleType>();
    }

    template <typename T, typename CollectionType>
    void try_add_system(std::unique_ptr<T> system) {
        if constexpr (std::is_base_of_v<CollectionType, T> || std::is_same_v<CollectionType, void>) {
            get_systems<CollectionType>().push_back(std::move(system));
        }
    }
};
}  // namespace tmt