#pragma once
#include <ImReflect.hpp>
#include "extern/implot/implot.h"

#include "engine/systems/audio/fourier.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "editor/imgui/types/glm.hpp"

inline void tag_invoke(ImReflect::ImInput_t, const char* label, tmt::Wave& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::Wave>();
    auto& type_response = response.get<tmt::Wave>();

    // Still let the user edit raw params if you want:
    ImReflect::Input("frequency", value.frequency, type_settings, type_response);
    ImReflect::Input("amplitude", value.amplitude, type_settings, type_response);
    ImReflect::Input("phase", value.phase, type_settings, type_response);

    // Sample the curve into an array
    float xs[100], ys[100];
    for (int i = 0; i < 100; i++) {
        float t = i / 99.0f;
        xs[i] = t;
        // ys[i] = glm::sin(t * value.frequency + value.phase) * value.amplitude;
        ys[i] = glm::sin(2.0f * glm::pi<float>() * t * value.frequency + value.phase) * value.amplitude;
    }

    ImGui::Text("%s", label);
    if (ImPlot::BeginPlot(label, ImVec2(-1, 150))) {
        ImPlot::SetupAxes("t", "value");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImGuiCond_Always);
        ImPlot::PlotLine("curve", xs, ys, 100);
        ImPlot::EndPlot();
    }
}

inline void tag_invoke(ImReflect::ImInput_t, const char*, tmt::Fourier& value, ImSettings& settings, ImResponse& response) {
    auto& type_settings = settings.get<tmt::Wave>();
    auto& type_response = response.get<tmt::Wave>();

    ImReflect::Input("data", value.data, type_settings, type_response);

    if (ImGui::Button("Construct Fourier")) {
        value.construct_fourier_curve();
    }

    bool fit_graphs = false;
    if (ImGui::Button("Fit Graphs")) {
        fit_graphs = true;
    }

    static FMOD::Sound* fourier_sound = nullptr;
    static FMOD::Channel* fourier_channel = nullptr;
    ImReflect::Input("Use Wave Data", value.use_wave_data, type_settings, type_response);
    if (ImGui::Button("Play")) {
        if (fourier_sound) fourier_sound->release();
        fourier_sound = value.make_fourier_sound(tmt::engine.audio.get_core_system());
        tmt::engine.audio.get_core_system()->playSound(fourier_sound, nullptr, false, &fourier_channel);
    }

    if (ImGui::Button("Play Chunks")) {
        if (fourier_sound) fourier_sound->release();
        fourier_sound = value.make_fourier_sound_from_chunks(tmt::engine.audio.get_core_system());
        tmt::engine.audio.get_core_system()->playSound(fourier_sound, nullptr, false, &fourier_channel);
    }

    if (value.animate) {
        float highest_frequency = detail;
        static bool forward = true;
        value.test_frequency = forward ? value.test_frequency + value.animation_time * highest_frequency : value.test_frequency - value.animation_time * highest_frequency;
        if (value.test_frequency > highest_frequency) forward = false;
        if (value.test_frequency < 0.0f) forward = true;

        for (int i = 0; i < detail; i++) {
            float t = (float)i / detail;
            float angle = t * 2.0f * glm::pi<float>() * value.test_frequency;
            glm::vec2 test_point = { glm::cos(angle), glm::sin(angle) };
            glm::vec2 sample_point = test_point * value.yf[i];
            value.xs[i] = sample_point.x;
            value.ys[i] = sample_point.y;
        }
    }

    ImReflect::Input("Waves Amount", value.highest_frequency_count, type_settings, type_response);
    if (ImGui::Button("Make Waves For Strongest Frequencies")) {
        value.waves.clear();
        for (int i = 0; i < value.highest_frequency_count && i < (int)value.peaks.size(); i++) {
            auto& peak = value.peaks[i];
            value.waves.push_back({ peak.freq, peak.magnitude, peak.phase });
        }
    }

    // fourier curve
    if (fit_graphs) ImPlot::SetNextAxesToFit();
    const char* label = "Fourier Curve";
    ImGui::Text("%s", label);
    if (ImPlot::BeginPlot(label, ImVec2(-1, 150))) {
        ImPlot::SetupAxes("t", "value");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImGuiCond_Always);
        ImPlot::PlotLine("curve", value.xf, value.yf, detail);
        ImPlot::PlotLine("smooth curve", value.xf, value.yh, detail);
        ImPlot::EndPlot();
    }

    // y coordinate of center of sphere fourier at all frequencies in a range, along the x axis
    if (fit_graphs) ImPlot::SetNextAxesToFit();
    label = "Strong Frequencies";
    ImGui::Text("%s", label);
    if (ImPlot::BeginPlot(label, ImVec2(-1, 500))) {
        ImPlot::SetupAxes("t", "value");
        ImPlot::PlotLine("curve", value.xc, value.yc, value.nyquist);
        ImPlot::PlotLine("smooth curve", value.xch, value.ych, value.nyquist);
        ImPlot::EndPlot();
    }

    // animation settings for sphere graph
    ImReflect::Input("test frequency", value.test_frequency, type_settings, type_response);
    ImReflect::Input("animate", value.animate, type_settings, type_response);
    ImReflect::Input("animation speed", value.animation_time, type_settings, type_response);

    // fourier sphere
    if (fit_graphs) ImPlot::SetNextAxesToFit();
    static float plot_height = 1000.0f;
    label = "Fourier Curve in Sphere";
    ImGui::Text("%s", label);
    if (ImPlot::BeginPlot(label, ImVec2(-plot_height, plot_height))) {
        ImPlot::SetupAxes("t", "value");
        // default
        ImPlot::PlotLine("curve", value.xs, value.ys, detail);
        glm::vec2 graph_center = { 0.0f, 0.0f };
        for (int i = 0; i < detail; i++) {
            graph_center += glm::vec2(value.xs[i], value.ys[i]);
        }
        graph_center /= (float)detail;
        ImPlot::PlotScatter("center", &graph_center.x, &graph_center.y, 1);

        // smoothed
        ImPlot::PlotLine("smooth curve", value.xsh, value.ysh, detail);
        graph_center = { 0.0f, 0.0f };
        for (int i = 0; i < detail; i++) {
            graph_center += glm::vec2(value.xsh[i], value.ysh[i]);
        }
        graph_center /= (float)detail;
        ImPlot::PlotScatter("center", &graph_center.x, &graph_center.y, 1);
        ImPlot::EndPlot();
    }
    
    if (ImGui::Button("Construct Chunks")) {
        value.construct_chunks_from_audio_data();
    }

    if (!value.chunks.empty()) {
        static int selected_chunk = 0;
        if (selected_chunk >= (int)value.chunks.size()) selected_chunk = 0;

        ImGui::SliderInt("Chunk", &selected_chunk, 0, (int)value.chunks.size() - 1);
        auto& c = value.chunks[selected_chunk];
        ImGui::Text("Time offset: %.3fs", c.time_offset);

        // per-chunk fourier curve (time domain)
        label = "Chunk Fourier Curve";
        ImGui::Text("%s", label);
        if (ImPlot::BeginPlot(label, ImVec2(-1, 150))) {
            ImPlot::SetupAxes("t", "value");
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImGuiCond_Always);
            ImPlot::PlotLine("curve", c.xf.data(), c.yf.data(), (int)c.xf.size());
            ImPlot::PlotLine("smooth curve", c.xf.data(), c.yh.data(), (int)c.yh.size());
            ImPlot::EndPlot();
        }

        // per-chunk spectrum
        label = "Chunk Strong Frequencies";
        ImGui::Text("%s", label);
        if (ImPlot::BeginPlot(label, ImVec2(-1, 500))) {
            ImPlot::SetupAxes("t", "value");
            ImPlot::PlotLine("curve", c.xc.data(), c.yc.data(), c.nyquist);
            ImPlot::PlotLine("smooth curve", c.xch.data(), c.ych.data(), c.nyquist);
            ImPlot::EndPlot();
        }

        // top peaks for this chunk
        if (ImGui::TreeNode("Peaks")) {
            int show_count = std::min((int)c.peaks.size(), 10);
            for (int i = 0; i < show_count; i++) {
                ImGui::Text("%.1f Hz  (mag %.4f)", c.peaks[i].freq, c.peaks[i].magnitude);
            }
            ImGui::TreePop();
        }
    }

    ImReflect::Input("waves", value.waves, type_settings, type_response);

    // --- Visible resize handle ---
    ImVec2 handle_pos = ImGui::GetCursorScreenPos();
    ImVec2 handle_size = ImVec2(ImGui::GetContentRegionAvail().x, 8.0f);

    ImGui::InvisibleButton("##resize_handle", handle_size);
    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();

    if (hovered || active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }

    if (active) {
        plot_height += ImGui::GetIO().MouseDelta.y;
        if (plot_height < 50.0f) plot_height = 50.0f;
    }

    // Draw the grip itself
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 grip_color = active ? IM_COL32(255, 255, 255, 180) : hovered ? IM_COL32(200, 200, 200, 140) : IM_COL32(120, 120, 120, 100);

    float center_y = handle_pos.y + handle_size.y * 0.5f;
    float bar_width = 40.0f;
    float center_x = handle_pos.x + handle_size.x * 0.5f;

    // Draw a small horizontal "grab bar" indicator
    draw_list->AddLine(ImVec2(center_x - bar_width * 0.5f, center_y), ImVec2(center_x + bar_width * 0.5f, center_y), grip_color, 3.0f);
}