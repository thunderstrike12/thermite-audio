#pragma once
#include "engine/tools/fmt/helper.hpp"

#include <filesystem>
#include <unordered_map>

FMT_LOGGING(std::filesystem::path, "{}", obj.string());

#include "engine/tools/serializer.hpp"

template <typename K, typename V, typename H>
struct fmt::formatter<std::unordered_map<K, V, H>> {
    constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const std::unordered_map<K, V>& obj, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", tmt::Serializer::serialize(obj));
    }
};