#pragma once
#include "engine/tools/fmt/helper.hpp"

#include "magic_enum/magic_enum.hpp"

template <typename E>
concept Enum = std::is_enum_v<E>;

template <Enum E>
struct fmt::formatter<E> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const E& obj, FormatContext& ctx) const {
        auto name = magic_enum::enum_name(obj);
        return fmt::format_to(ctx.out(), "{}", name);
    }
};