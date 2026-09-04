#pragma once

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

#include "engine/core/resources.hpp"
#include "engine/core/reflection.hpp"
#include "audio_data.hpp"
#include <fmod.hpp>

constexpr int detail = 500;
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

struct Chunk {
    float time_offset;            // seconds into the audio this chunk starts at
    int nyquist;                  // valid bin count for this chunk
    std::vector<float> xc, yc;    // raw spectrum
    std::vector<float> ych;       // smoothed spectrum
    std::vector<float> xcr, ycr;  // spike spectrum from peaks
    std::vector<float> xf, yf;    // fourier curve
    std::vector<float> yh;        // smoothed fourier curve
    std::vector<float> yr;        // reconstructed fourier curve from peaks
    std::vector<float> phase_h;   // phase of the smoothed spectrum
    std::vector<Peak> peaks;      // peaks found in this chunk
};

class Fourier {
   public:
    Fourier() {};

    std::vector<Wave> waves;
    std::vector<Chunk> chunks;
    int max_peaks_per_chunk = 10;

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
    void construct_chunks_from_audio_data();
    void construct_fourier_curve();

    FMOD::Sound* make_fourier_sound_from_audio_data(FMOD::System* system);
    FMOD::Sound* make_fourier_sound_from_waves(FMOD::System* system);
    FMOD::Sound* make_fourier_sound_from_chunks(FMOD::System* system);
    FMOD::Sound* make_fourier_sound_from_single_chunk(FMOD::System* system, const Chunk& c, int repeat_count);
};

}  // namespace tmt

TMT_COMPONENT(tmt::Wave, "Wave", (frequency, amplitude, phase));
TMT_COMPONENT(tmt::Fourier, "Fourier", (test_frequency, waves, data));