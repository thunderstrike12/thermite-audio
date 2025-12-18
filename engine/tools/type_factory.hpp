#pragma once
#include <memory>
#include <functional>

namespace tmt {

template <typename T>
class TypeFactory {
   public:
    using Type = T;

    template <typename U>
    void register_type() {
        factory = []() { return U(); };
    }

    T create() { return factory(); }

   private:
    std::function<T()> factory;
};

template <typename T>
class TypeFactory<std::unique_ptr<T>> {
   public:
    using Type = std::unique_ptr<T>;

    template <typename U>
    void register_type() {
        factory = []() -> T* { return new U(); };
    }

    std::unique_ptr<T> create() { return std::unique_ptr<T>(factory()); }

   private:
    std::function<T*()> factory;
};
}  // namespace tmt