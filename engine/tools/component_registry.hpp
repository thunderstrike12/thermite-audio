#pragma once
#include <tuple>

namespace tmt {

template <typename T>
struct type_tag {
    using type = T;
};

template <typename... Components>
class ComponentRegistry {
   public:
    using components = std::tuple<Components...>;

    static constexpr size_t COUNT = sizeof...(Components);

    template <size_t Index>
    using get = std::tuple_element_t<Index, components>;

    template <typename Func, size_t... Is>
    static void for_each_impl(Func&& func, std::index_sequence<Is...>) {
        // Use void cast to force statement expression context
        (void(func(type_tag<std::tuple_element_t<Is, components>> {})), ...);
    }

    template <typename Func>
    static void for_each(Func&& func) {
        for_each_impl(std::forward<Func>(func), std::make_index_sequence<COUNT> {});
    }
};
}  // namespace tmt