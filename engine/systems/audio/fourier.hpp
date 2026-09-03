#pragma once

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/reflection.hpp"
#include "audio_data.hpp"
#include <fmod.hpp>

constexpr int detail = 5000;
constexpr int render_detail = 44100;
constexpr int max_nyquist = 24000;

namespace tmt {

struct Peak {
    float freq;
    float magnitude;
    float phase;
};

class Wave {
   public:
    float frequency = 0.0f;
    float amplitude = 0.0f;
    float phase = 0.0f;
};

class Fourier {
   public:
    Fourier() {};

    std::vector<Wave> waves;

    int nyquist = 0;

    float xf_render[render_detail], yf_render[render_detail];

    float xf[detail], yf[detail];
    float yh[detail];
    float xc[max_nyquist], yc[max_nyquist];
    float xch[max_nyquist], ych[max_nyquist];
    float xs[detail], ys[detail];
    float xsh[detail], ysh[detail];

    std::vector<Peak> peaks;
    int highest_frequency_count = 10;
    float test_frequency = 1.0f;
    float animation_time = 0.0f;
    bool animate = false;
    bool use_wave_data = false;
    bool constructed = false;
    ResourceRef<AudioData> data;
    void construct_fourier_curve();
    FMOD::Sound* make_fourier_sound(FMOD::System* system, int sampleRate = 44100);
};

}  // namespace tmt

TMT_COMPONENT(tmt::Wave, "Wave", (frequency, amplitude, phase));
TMT_COMPONENT(tmt::Fourier, "Fourier", (test_frequency, waves, data));