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

    const char* label;
    bool fit_graphs = false;
    static FMOD::Sound* fourier_sound_audio = nullptr;
    static FMOD::Channel* fourier_channel_audio = nullptr;
    static FMOD::Sound* fourier_sound_waves = nullptr;
    static FMOD::Channel* fourier_channel_waves = nullptr;
    static FMOD::Sound* fourier_sound_chunks = nullptr;
    static FMOD::Channel* fourier_channel_chunks = nullptr;

    static FMOD::Sound* fourier_sound_live = nullptr;
    static FMOD::Channel* fourier_channel_live = nullptr;
    static bool live_playing = false;
    static int last_live_chunk = -1;

    if (ImGui::Button("Load Audio Sound")) {
        if (fourier_sound_audio) fourier_sound_audio->release();
        fourier_sound_audio = value.make_fourier_sound_from_audio_data(tmt::engine.audio.get_core_system());
        tmt::engine.audio.get_core_system()->playSound(fourier_sound_audio, nullptr, false, &fourier_channel_audio);
    }

    if (ImGui::Button("Play Audio Sound")) {
        if (fourier_channel_audio) fourier_channel_audio->stop();
        if (fourier_sound_audio) {
            tmt::engine.audio.get_core_system()->playSound(fourier_sound_audio, nullptr, false, &fourier_channel_audio);
        }
    }

    if (ImGui::Button("Stop all playing sounds")) {
        if (fourier_channel_audio) fourier_channel_audio->stop();
        if (fourier_channel_chunks) fourier_channel_chunks->stop();
        if (fourier_channel_waves) fourier_channel_waves->stop();
    }

    if (ImGui::CollapsingHeader("Manual Fourier Construction")) {
        ImReflect::Input("Use Wave Data", value.use_wave_data, type_settings, type_response);
        if (ImGui::Button("Construct Fourier")) {
            value.construct_fourier_curve();
        }

        if (ImGui::Button("Fit Graphs")) {
            fit_graphs = true;
        }

        if (ImGui::Button("Load Wave Sound")) {
            if (fourier_sound_waves) fourier_sound_waves->release();
            fourier_sound_waves = value.make_fourier_sound_from_waves(tmt::engine.audio.get_core_system());
            tmt::engine.audio.get_core_system()->playSound(fourier_sound_waves, nullptr, false, &fourier_channel_waves);
        }

        if (ImGui::Button("Play Wave Sound")) {
            if (fourier_channel_waves) fourier_channel_waves->stop();
            if (fourier_sound_waves) {
                tmt::engine.audio.get_core_system()->playSound(fourier_sound_waves, nullptr, false, &fourier_channel_waves);
            }
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
        label = "Fourier Curve";
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
            glm::vec2 graph_center_smooth = { 0.0f, 0.0f };
            for (int i = 0; i < detail; i++) {
                graph_center_smooth += glm::vec2(value.xsh[i], value.ysh[i]);
            }
            graph_center_smooth /= (float)detail;
            ImPlot::PlotScatter("smooth center", &graph_center_smooth.x, &graph_center_smooth.y, 1);
            ImPlot::EndPlot();
        }

        if (ImGui::CollapsingHeader("Waves")) {
            ImReflect::Input("waves", value.waves, type_settings, type_response);
        }
    }

    if (ImGui::CollapsingHeader("Chunk Generation And Playback")) {
        fit_graphs = false;
        if (ImGui::Button("Fit Chunk Graphs")) {
            fit_graphs = true;
        }

        if (ImGui::Button("Construct Chunks")) {
            value.construct_chunks_from_audio_data();
        }
        ImReflect::Input("max peaks per chunk", value.max_peaks_per_chunk, type_settings, type_response);

        if (!value.chunks.empty()) {
            if (ImGui::Button("Load Chunk Sound")) {
                if (fourier_sound_chunks) fourier_sound_chunks->release();
                fourier_sound_chunks = value.make_fourier_sound_from_chunks(tmt::engine.audio.get_core_system());
                tmt::engine.audio.get_core_system()->playSound(fourier_sound_chunks, nullptr, false, &fourier_channel_chunks);
            }

            if (ImGui::Button("Play Chunk Sound")) {
                if (fourier_channel_chunks) fourier_channel_chunks->stop();
                if (fourier_sound_chunks) {
                    tmt::engine.audio.get_core_system()->playSound(fourier_sound_chunks, nullptr, false, &fourier_channel_chunks);
                }
            }

            static int selected_chunk = 0;
            if (selected_chunk >= (int)value.chunks.size()) selected_chunk = 0;

            ImGui::SliderInt("Chunk", &selected_chunk, 0, (int)value.chunks.size() - 1);
            auto& c = value.chunks[selected_chunk];
            ImGui::Text("Time offset: %.3fs", c.time_offset);

            if (ImGui::Button(live_playing ? "Stop Live Chunk Playback" : "Start Live Chunk Playback")) {
                live_playing = !live_playing;
                if (!live_playing) {
                    if (fourier_channel_live) fourier_channel_live->stop();
                    if (fourier_sound_live) {
                        fourier_sound_live->release();
                        fourier_sound_live = nullptr;
                    }
                    last_live_chunk = -1;
                }
            }

            if (live_playing  && selected_chunk != last_live_chunk) {
                if (fourier_channel_live) fourier_channel_live->stop();
                if (fourier_sound_live) {
                    fourier_sound_live->release();
                    fourier_sound_live = nullptr;
                }

                fourier_sound_live = value.make_fourier_sound_from_single_chunk(tmt::engine.audio.get_core_system(), c, 20);
                tmt::engine.audio.get_core_system()->playSound(fourier_sound_live, nullptr, false, &fourier_channel_live);

                last_live_chunk = selected_chunk;
            }

            // per-chunk fourier curve (time domain)
            if (fit_graphs) ImPlot::SetNextAxesToFit();
            label = "Chunk Fourier Curve";
            ImGui::Text("%s", label);
            if (ImPlot::BeginPlot(label, ImVec2(-1, 150))) {
                ImPlot::SetupAxes("t", "value");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImGuiCond_Always);
                ImPlot::PlotLine("curve", c.xf.data(), c.yf.data(), (int)c.xf.size());
                ImPlot::PlotLine("smooth curve", c.xf.data(), c.yh.data(), (int)c.yh.size());
                ImPlot::PlotLine("reconstructed curve", c.xf.data(), c.yr.data(), (int)c.yr.size());
                ImPlot::EndPlot();
            }

            int top_n = std::min(value.max_peaks_per_chunk, (int)c.peaks.size());
            c.xcr.resize(top_n * 3);
            c.ycr.resize(top_n * 3);

            for (int j = 0; j < top_n; j++) {
                auto& peak = c.peaks[j];
                c.xcr[j * 3] = peak.freq;
                c.ycr[j * 3] = 0.0f;
                c.xcr[j * 3 + 1] = peak.freq;
                c.ycr[j * 3 + 1] = peak.magnitude;
                c.xcr[j * 3 + 2] = peak.freq;
                c.ycr[j * 3 + 2] = 0.0f;
            }

            // per-chunk spectrum
            if (fit_graphs) ImPlot::SetNextAxesToFit();
            label = "Chunk Strong Frequencies";
            ImGui::Text("%s", label);
            if (ImPlot::BeginPlot(label, ImVec2(-1, 500))) {
                ImPlot::SetupAxes("t", "value");
                ImPlot::PlotLine("curve", c.xc.data(), c.yc.data(), c.nyquist);
                ImPlot::PlotLine("smooth curve", c.xc.data(), c.ych.data(), c.nyquist);
                ImPlot::PlotLine("frequencies present", c.xcr.data(), c.ycr.data(), (int)c.xcr.size());
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
    }
}