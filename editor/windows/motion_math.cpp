#include "motion_math.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/motion_math/motion_math_system.hpp"
#include <imgui.h>

#include "implot.h"

void tmt::MotionMathPreview::display() {
    auto& motion_math_sys = tmt::engine.ecs.systems.get<tmt::MotionMathSystem>();

    recompute |= ImGui::SliderFloat("Frequency", &frequency, 0.f, 10.f);
    recompute |= ImGui::SliderFloat("Damping", &damping, 0.f, 10.f);
    recompute |= ImGui::SliderFloat("Initial response", &initial_response, -10.f, 10.f);
    recompute |= ImGui::InputFloat("Target", &target);

    if (recompute) {
        recompute = false;

        float incr = 1.f / static_cast<float>(NUM_SAMPLES);

        SecondOrderSolver::State<float> state {};
        state.current_state = 0.f;

        for (uint32_t i = 0; i < NUM_SAMPLES; i++) {
            x_data[i] = static_cast<float>(i) * incr;
            SecondOrderSolver::solve(state, target, frequency, damping, initial_response, Engine::Config::FIXED_TIME_STEP);
            y_data[i] = state.current_state;
        }
    }

    if (ImPlot::BeginPlot("Preview")) {
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.f, 1.f, ImPlotCond_Once);
        ImPlot::SetupAxisLimits(ImAxis_Y1, -1.2f, 1.2f, ImPlotCond_Once);

        ImPlot::PlotLine("Output", x_data, y_data, NUM_SAMPLES);

        ImPlot::EndPlot();
    }
}
