#pragma once

#include <ImReflect.hpp>

#include <engine/core/components/emitter.hpp>

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::ParticleEffect& effect, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::ParticleEffect>();
    auto& type_response = response.get<tmt::ParticleEffect>();

    ImReflect::Input("Active", effect.active, type_settings, type_response);
    ImReflect::Input("Lifetime", effect.particle_lifetime, type_settings, type_response);
    ImReflect::Input("Pos Offset", effect.pos_offset, type_settings, type_response);
    ImReflect::Input("Direction", effect.dir, type_settings, type_response);
    ImReflect::Input("Cone Angle", effect.cone_angle, type_settings, type_response);
    ImReflect::Input("Spawn Count", effect.spawn_count, type_settings, type_response);
    
    ImReflect::Input("Animation Speed", effect.anim_speed, type_settings, type_response);
    ImReflect::Input("Dither Scale", effect.dither_scale, type_settings, type_response);

    if (ImGui::TreeNode("Speed")) {
        ImReflect::Input("Start Speed", effect.start_speed, type_settings, type_response);
        ImReflect::Input("End Speed", effect.end_speed, type_settings, type_response);
        ImReflect::Input("Speed Curve", effect.speed_curve, type_settings, type_response);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Size")) {
        ImReflect::Input("Start Size", effect.start_size, type_settings, type_response);
        ImReflect::Input("End Size", effect.end_size, type_settings, type_response);
        ImReflect::Input("Size Curve", effect.size_curve, type_settings, type_response);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Opacity")) {
        ImReflect::Input("Start Opacity", effect.start_opacity, type_settings, type_response);
        ImReflect::Input("End Opacity", effect.end_opacity, type_settings, type_response);
        ImReflect::Input("Opacity Curve", effect.opacity_curve, type_settings, type_response);
        ImGui::TreePop();
    }

    ImReflect::Input("Rotation", effect.rotation, type_settings, type_response);
    ImReflect::Input("Position Jitter", effect.pos_jitter, type_settings, type_response);
    ImReflect::Input("Jitter Speed", effect.jitter_speed, type_settings, type_response);
    ImReflect::Input("Texture", effect.texture, type_settings, type_response);
}

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::ParticleEmitter& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::ParticleEmitter>();
    auto& type_response = response.get<tmt::ParticleEmitter>();

    ImReflect::Input("Emitter Active", value.active, type_settings, type_response);

    if (ImGui::Button("Emitter Burst")) {
        value.should_burst = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /* Add Effect Button */
    if (ImGui::Button("+ Add Effect")) {
        value.effects.emplace_back();
        value.effects.back().name = "Effect " + std::to_string(value.effects.size());
    }

    ImGui::Spacing();

    /* Effect List */
    int remove_idx = -1;
    int move_up_idx = -1;
    int move_down_idx = -1;

    for (int i = 0; i < value.effects.size(); i++) {
        auto& effect = value.effects[i];
        ImGui::PushID(i);

        /* Header with colored accent */
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(effect.name_color.x, effect.name_color.y, effect.name_color.z, 1.0f));
        bool open = ImGui::CollapsingHeader(effect.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
        ImGui::PopStyleColor();

        /* Right-aligned buttons on the header line */
        float button_width = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float arrow_width = ImGui::GetFrameHeight();
        float total_width = button_width + arrow_width * 2.0f + ImGui::GetStyle().ItemSpacing.x * 2.0f;

        ImGui::SameLine(ImGui::GetContentRegionAvail().x - total_width + ImGui::GetStyle().ItemSpacing.x);

        bool can_move_up = i > 0;
        bool can_move_down = i < value.effects.size() - 1;

        ImGui::BeginDisabled(!can_move_up);
        if (ImGui::ArrowButton("##up", ImGuiDir_Up)) {
            move_up_idx = i;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!can_move_down);
        if (ImGui::ArrowButton("##down", ImGuiDir_Down)) {
            move_down_idx = i;
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("X")) {
            remove_idx = i;
        }
        ImGui::PopStyleColor(2);

        if (open) {
            ImGui::Indent();

            /* Editable name field */
            char name_buf[128];
            strncpy(name_buf, effect.name.c_str(), sizeof(name_buf));
            name_buf[sizeof(name_buf) - 1] = '\0';
            if (ImGui::InputText("Name", name_buf, sizeof(name_buf))) {
                effect.name = name_buf;
            }

            ImVec4 color {};
            color = { effect.name_color.x, effect.name_color.y, effect.name_color.z, 1.0f };
            if (ImGui::ColorEdit3("Name Color", &color.x)) {
                effect.name_color = { color.x, color.y, color.z };
            }

            ImGui::Spacing();

            if (ImGui::Button("Effect Burst")) {
                value.should_burst = true;
                effect.should_burst = true;
            }
            ImReflect::Input("##effect", effect, type_settings, type_response);

            ImGui::Unindent();
        }

        ImGui::PopID();
    }

    /* Apply deferred modifications */
    if (remove_idx >= 0) {
        value.effects.erase(value.effects.begin() + remove_idx);
    }
    if (move_up_idx > 0) {
        std::swap(value.effects[move_up_idx], value.effects[move_up_idx - 1]);
    }
    if (move_down_idx >= 0 && move_down_idx < value.effects.size() - 1) {
        std::swap(value.effects[move_down_idx], value.effects[move_down_idx + 1]);
    }
}