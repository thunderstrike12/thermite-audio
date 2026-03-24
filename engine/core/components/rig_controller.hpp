#pragma once

#include <string>
#include <variant>
#include <map>

#include "engine/core/ecs.hpp"

#ifdef THERMITE_EDITOR
// NOLINTBEGIN(readability-identifier-naming)
namespace ax {

namespace NodeEditor {

enum class SaveReasonFlags : uint32_t;

struct EditorContext;

}  // namespace NodeEditor

}  // namespace ax
// NOLINTEND(readability-identifier-naming)
#endif

namespace tmt {

struct Trigger {
    Trigger() = default;
    Trigger(const bool value) : value(value) {}

    // operators needed due to comparing std::variant<..., Trigger>
    [[nodiscard]] bool operator==(const Trigger&) const { return value; }
    [[nodiscard]] bool operator!=(const Trigger&) const { return value; }
    [[nodiscard]] bool operator>(const Trigger&) const { return value; }
    [[nodiscard]] bool operator<(const Trigger&) const { return value; }

    bool value = false;
};

struct AnimationState {
    std::string animation;

    float animation_speed = 1.0f;
    bool repeat = false;
    bool transition_on_finish = false;  // Only transition after finishing the animation.
};

using Variant = std::variant<float, int, bool, Trigger>;

struct Condition {
    enum class CompareType : uint8_t { EQUAL = 0, NOT_EQUAL = 1, GREATER = 2, LESS = 3 };
    using CompareFunctionMap = std::map<CompareType, std::function<bool(const Variant&, const Variant&)>>;

    static const CompareFunctionMap COMPARE_MAP;

    [[nodiscard]] bool is_satisfied(const Variant& parameter) const {
        const auto& compare = COMPARE_MAP.at(compare_type);
        const bool satisfied = compare(parameter, compare_value);
        return satisfied;
    }

    std::string parameter_name;

    Variant compare_value;
    CompareType compare_type;
};

struct Transition {
    std::string from;
    std::string to;

    float transition_time = 0.0f;
    std::vector<Condition> conditions {};
};

class RigController {
   public:
    void set_parameter_float(const std::string& name, float value);
    void set_parameter_int(const std::string& name, int value);
    void set_parameter_bool(const std::string& name, bool value);
    void set_parameter_trigger(const std::string& name, bool value = true);

    [[nodiscard]] float get_parameter_float(const std::string& name) const;
    [[nodiscard]] int get_parameter_int(const std::string& name) const;
    [[nodiscard]] bool get_parameter_bool(const std::string& name) const;
    [[nodiscard]] bool get_parameter_trigger(const std::string& name) const;

    std::map<std::string, Variant> parameters;
    std::map<std::string, AnimationState> states = { { "Start", AnimationState {} } };
    std::vector<Transition> transitions;

#ifdef THERMITE_EDITOR
    ax::NodeEditor::EditorContext* editor_context { nullptr };
    std::string editor_data;
#endif  // THERMITE_EDITOR

   private:
    friend class RigStateController;
    friend class RigModelManager;

    std::string current_state = "Start";

    void remove_state(const std::string& name);
    void remove_parameter(const std::string& name);

    void rename_state(const std::string& old_name, const std::string& new_name);
    void rename_parameter(const std::string& old_name, const std::string& new_name);

    bool check_transition_conditions(const Transition& transition, std::vector<Trigger*>& satisfied_triggers);
};

}  // namespace tmt

JSON_REFLECT(tmt::Trigger, value);
JSON_REFLECT(tmt::AnimationState, animation, animation_speed, repeat, transition_on_finish);
JSON_REFLECT(tmt::Transition, from, to, transition_time, conditions);
JSON_REFLECT(tmt::Condition, parameter_name, compare_value, compare_type);
JSON_REFLECT(tmt::RigController, parameters, states, transitions);
TMT_COMPONENT_NAME(tmt::RigController, "Rig Controller");