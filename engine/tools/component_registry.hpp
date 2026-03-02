#pragma once
#include <tuple>

namespace tmt {

template <typename T>
struct type_tag {
    using type = T;
};

template <typename... all_components>
class ComponentRegistry {
   public:
    using Components = std::tuple<all_components...>;

    static constexpr size_t COUNT = sizeof...(all_components);

    template <size_t Index>
    using get = std::tuple_element_t<Index, Components>;

    template <typename T>
    static constexpr bool contains() {
        return (std::is_same_v<T, all_components> || ...);
    }

    template <typename Func, size_t Index = 0>
    static void for_each(Func&& func) {
        if constexpr (Index < COUNT) {
            func(type_tag<get<Index>> {});
            /* Recurse to the next index */
            for_each<Func, Index + 1>(std::forward<Func>(func));
        }
    }
};

}  // namespace tmt