#pragma once
#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <ImReflect.hpp>
#include <unordered_set>
#include <algorithm>
#include <cctype>

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"

#include "editor/font/icon_lookups.hpp"
#include "editor/windows/hierarchy.hpp"

/* Custom response type for entity */
template <>
struct ImReflect::type_response<entt::entity> : ImReflect::Detail::required_response<entt::entity> {
   private:
    bool is_dropped = false;

   public:
    void dropped() { is_dropped = true; }
    bool is_entity_dropped() const { return is_dropped; }
};

/* Main input implementation for mutable entity reference */
void tag_invoke(ImReflect::ImInput_t, const char* label, entt::entity& value, ImSettings& settings, ImResponse& response);

/* Const version, read only display */
void tag_invoke(ImReflect::ImInput_t, const char* label, const entt::entity& value, ImSettings& settings, ImResponse& response);
