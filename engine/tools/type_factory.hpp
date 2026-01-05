#pragma once
#include <memory>
#include <functional>
#include <any>
#include <tuple>
#include <utility>

namespace tmt {

template <typename T>
class TypeFactory {
   public:
    template <typename U, typename... CtorArgs>
    void register_type() {
        factory = [](std::any args_any) -> T {
            auto args = std::any_cast<std::tuple<std::decay_t<CtorArgs>...>>(args_any);
            return std::apply([](auto&&... args) { return U(std::forward<decltype(args)>(args)...); }, std::move(args));
        };
    }

    template <typename... Args>
    T create(Args&&... args) {
        return factory(std::make_any<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...));
    }

   private:
    std::function<T(std::any)> factory;
};

template <typename T>
class TypeFactory<std::unique_ptr<T>> {
   public:
    using Type = std::unique_ptr<T>;

    template <typename U, typename... CtorArgs>
    void register_type() {
        factory = [](std::any args_any) -> Type {
            auto args = std::any_cast<std::tuple<std::decay_t<CtorArgs>...>>(args_any);
            return std::apply([](auto&&... args) { return std::make_unique<U>(std::forward<decltype(args)>(args)...); }, std::move(args));
        };
    }

    template <typename... Args>
    Type create(Args&&... args) {
        return factory(std::make_any<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...));
    }

   private:
    std::function<Type(std::any)> factory;
};
}  // namespace tmt