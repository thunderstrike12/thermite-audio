#pragma once
#include <tuple>

namespace tmt {

/* Forward declare */
template <typename... all_components>
class ComponentRegistry;

template <typename T, typename Registry>
struct type_tag {
    using type = T;
    using registry = Registry;
};

template <typename T>
struct is_component_registry : std::false_type {};

template <typename... Components>
struct is_component_registry<ComponentRegistry<Components...>> : std::true_type {};

template <typename T>
inline constexpr bool IS_COMPONENT_REGISTRY_V = is_component_registry<T>::value;

template <typename... all_components>
class ComponentRegistry {
   public:
    using Components = std::tuple<all_components...>;

    static constexpr size_t COUNT = sizeof...(all_components);

    template <size_t Index>
    using get = std::tuple_element_t<Index, Components>;

    template <typename T>
    static constexpr bool contains() {
        /* Check if T is in the list of all_components */
        constexpr bool THIS_RESULT = (std::is_same_v<T, all_components> || ...);
        if constexpr (THIS_RESULT) {
            return true;
        }
        /* If not, check if any nested registry contains T */
        constexpr bool NESTED_RESULT = ((IS_COMPONENT_REGISTRY_V<all_components> && all_components::template contains<T>()) || ...);
        return NESTED_RESULT;
    }

    template <typename Func, size_t Index = 0>
    static void for_each(Func&& func) {
        if constexpr (Index < COUNT) {
            using ComponentType = get<Index>;
            constexpr bool IS_REGISTRY = IS_COMPONENT_REGISTRY_V<ComponentType>;
            if constexpr (IS_REGISTRY) {
                /* If it's a nested registry, recurse into it */
                ComponentType::for_each(std::forward<Func>(func));
            } else {
                func(type_tag<ComponentType, ComponentRegistry<all_components...>> {});
            }
            /* Recurse to the next index */
            for_each<Func, Index + 1>(std::forward<Func>(func));
        }
    }
};

}  // namespace tmt