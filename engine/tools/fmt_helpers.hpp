#pragma once
#include <spdlog/fmt/fmt.h>

// Updated macro with const format method
#define FMT_LOGGING(T, format_string, ...)                                           \
    template <>                                                                      \
    struct fmt::formatter<T> {                                                       \
        constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); } \
        template <typename FormatContext>                                            \
        auto format(const T& obj, FormatContext& ctx) const {                        \
            return fmt::format_to(ctx.out(), format_string, __VA_ARGS__);            \
        }                                                                            \
    }