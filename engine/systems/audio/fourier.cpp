#include "fourier.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/components/transform.hpp"
#include "engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/resources.hpp"
#include "engine/core/components/voxel_renderer.hpp"

namespace {}

namespace tmt {

void Fourier::construct_chunks_from_audio_data() {
    chunks.clear();
    if (data == nullptr) return;

    int total_samples = (int)data->samples.size();
    int chunk_nyquist = std::min(data->sample_rate / 2, max_nyquist);

    for (int start = 0; start + detail <= total_samples; start += detail) {
        Chunk c;
        c.time_offset = (float)start / (float)data->sample_rate;
        c.nyquist = chunk_nyquist;

        c.xf.resize(detail);
        c.yf.resize(detail);
        c.yh.resize(detail);
        c.xc.resize(c.nyquist);
        c.yc.resize(c.nyquist);
        c.xch.resize(c.nyquist);
        c.ych.resize(c.nyquist);

        // fourier curve for this chunk
        for (int i = 0; i < detail; i++) {
            float t = (float)i / (float)detail;
            c.xf[i] = t;
            c.yf[i] = data->samples[start + i];
        }

        // hann window
        for (int i = 0; i < detail; i++) {
            float t = (float)i / ((float)detail - 1.0f);
            float w = 0.5f * (1.0f - glm::cos(2.0f * glm::pi<float>() * t));
            c.yh[i] = c.yf[i] * w;
        }

        // frequency sweep, raw + smoothed
        for (int j = 0; j < c.nyquist; j++) {
            float test_freq = (float)j;
            float real_sum = 0.0f, imag_sum = 0.0f;
            float real_sum_h = 0.0f, imag_sum_h = 0.0f;
            for (int i = 0; i < detail; i++) {
                float t = (float)i / (float)data->sample_rate;
                float angle = t * 2.0f * glm::pi<float>() * test_freq;
                float cs = glm::cos(angle), sn = glm::sin(angle);
                real_sum += c.yf[i] * cs;
                imag_sum += c.yf[i] * sn;
                real_sum_h += c.yh[i] * cs;
                imag_sum_h += c.yh[i] * sn;
            }
            c.xc[j] = test_freq;
            c.yc[j] = glm::sqrt(real_sum * real_sum + imag_sum * imag_sum) / detail;
            c.xch[j] = test_freq;
            c.ych[j] = glm::sqrt(real_sum_h * real_sum_h + imag_sum_h * imag_sum_h) / detail;
        }

        // peaks for this chunk
        for (int j = 1; j < c.nyquist - 1; j++) {
            if (c.ych[j] > c.ych[j - 1] && c.ych[j] > c.ych[j + 1]) {
                c.peaks.push_back({ c.xch[j], c.ych[j], 0.0f });
            }
        }
        std::sort(c.peaks.begin(), c.peaks.end(), [](const Peak& a, const Peak& b) { return a.magnitude > b.magnitude; });

        chunks.push_back(std::move(c));
    }
}

void Fourier::construct_fourier_curve() {
    constructed = true;

    nyquist = use_wave_data ? (detail / 2) : (data->sample_rate / 2);

    // fourier curve
    if (use_wave_data) {
        // fourier made up of waves
        for (int j = 0; j < (int)waves.size(); j++) {
            for (int i = 0; i < detail; i++) {
                float t = (float)i / (float)detail;
                xf[i] = t;
                if (j == 0) {
                    yf[i] = glm::sin(2.0f * glm::pi<float>() * t * waves[j].frequency + waves[j].phase) * waves[j].amplitude;
                } else {
                    yf[i] += glm::sin(2.0f * glm::pi<float>() * t * waves[j].frequency + waves[j].phase) * waves[j].amplitude;
                }
            }
        }
        // wave reconstruction - higher detail for audio playback
        for (int i = 0; i < render_detail; i++) {
            float t = (float)i / (float)render_detail;
            xf_render[i] = t;
            float sample = 0.0f;
            for (auto& w : waves) sample += glm::sin(2.0f * glm::pi<float>() * t * w.frequency + w.phase) * w.amplitude;
            yf_render[i] = sample;
        }
    } else {
        // fourier made up of audio data
        for (int i = 0; i < detail; i++) {
            float t = (float)i / (float)detail;
            xf[i] = t;
            yf[i] = data->samples[i];
        }
    }

    // apply hann window
    for (int i = 0; i < detail; i++) {
        float t = (float)i / ((float)detail - 1.0f);
        float smooth_window = 0.5f * (1.0f - glm::cos(2.0f * glm::pi<float>() * t));
        yh[i] = yf[i] * smooth_window;
    }

    // sphere fourier at a certain test frequency
    for (int i = 0; i < detail; i++) {
        float t = (float)i / detail;
        float angle = t * 2.0f * glm::pi<float>() * test_frequency;
        glm::vec2 test_point = { glm::cos(angle), glm::sin(angle) };
        glm::vec2 sample_point = test_point * yf[i];
        xs[i] = sample_point.x;
        ys[i] = sample_point.y;
    }

    // sphere fourier at a certain test frequency - using smoothed fourier
    for (int i = 0; i < detail; i++) {
        float t = (float)i / detail;
        float angle = t * 2.0f * glm::pi<float>() * test_frequency;
        glm::vec2 test_point = { glm::cos(angle), glm::sin(angle) };
        glm::vec2 sample_point = test_point * yh[i];
        xsh[i] = sample_point.x;
        ysh[i] = sample_point.y;
    }

    // y coordinate of center of sphere fourier at all frequencies in a range, along the x axis
    for (int j = 0; j < nyquist; j++) {
        float test_freq = (float)j;
        float real_sum = 0.0f, imag_sum = 0.0f;
        for (int i = 0; i < detail; i++) {
            float t;
            if (use_wave_data) {
                t = (float)i / (float)detail;
            } else {
                t = (float)i / (float)data->sample_rate;
            }
            float angle = t * 2.0f * glm::pi<float>() * test_freq;
            real_sum += yf[i] * glm::cos(angle);
            imag_sum += yf[i] * glm::sin(angle);
        }
        xc[j] = test_freq;
        yc[j] = glm::sqrt(real_sum * real_sum + imag_sum * imag_sum) / detail;
    }

    // y coordinate of center of sphere fourier at all frequencies in a range, along the x axis - using smoothed fourier
    for (int j = 0; j < nyquist; j++) {
        float test_freq = (float)j;
        float real_sum = 0.0f, imag_sum = 0.0f;
        for (int i = 0; i < detail; i++) {
            float t;
            if (use_wave_data) {
                t = (float)i / (float)detail;
            } else {
                t = (float)i / (float)data->sample_rate;
            }

            float angle = t * 2.0f * glm::pi<float>() * test_freq;
            real_sum += yh[i] * glm::cos(angle);
            imag_sum += yh[i] * glm::sin(angle);
        }
        xch[j] = test_freq;
        ych[j] = glm::sqrt(real_sum * real_sum + imag_sum * imag_sum) / detail;
    }

    // find peaks in the smooth sphere center graph
    for (int j = 1; j < nyquist - 1; j++) {
        if (ych[j] > ych[j - 1] && ych[j] > ych[j + 1]) {
            peaks.push_back({ xch[j], ych[j] });
        }
    }

    std::sort(peaks.begin(), peaks.end(), [](const Peak& a, const Peak& b) { return a.magnitude > b.magnitude; });
}

FMOD::Sound* Fourier::make_fourier_sound(FMOD::System* system) {
    int period_samples = (int)(data->sample_rate);  // samples in one full period

    std::vector<float> buffer(period_samples, 0.0f);
    for (int i = 0; i < period_samples; i++) {
        float t = (float)i / data->sample_rate;  // real time in seconds
        float sample = 0.0f;
        if (use_wave_data) {
            for (auto& wave : waves) {
                float freq_hz = wave.frequency;
                sample += glm::sin(2.0f * glm::pi<float>() * freq_hz * t + wave.phase) * wave.amplitude;
            }
        } else {
            sample += data->samples[i % data->samples.size()];
        }
        buffer[i] = sample;
    }

    // Normalize so we don't clip if harmonics stack above 1.0
    float max_abs = 0.0f;
    for (float s : buffer) max_abs = std::max(max_abs, std::abs(s));
    if (max_abs > 1.0f)
        for (float& s : buffer) s /= max_abs;

    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(exinfo);
    exinfo.numchannels = 1;
    exinfo.defaultfrequency = data->sample_rate;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.length = (unsigned int)(buffer.size() * sizeof(float));

    FMOD::Sound* sound = nullptr;
    // system->createSound((const char*)buffer.data(), FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE, &exinfo, &sound);
    system->createSound((const char*)buffer.data(), FMOD_OPENMEMORY | FMOD_OPENRAW | FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE, &exinfo, &sound);

    // FMOD::Channel* channel = nullptr;
    // system->playSound(sound, nullptr, false, &channel);

    return sound;
}

FMOD::Sound* Fourier::make_fourier_sound_from_chunks(FMOD::System* system) {
    if (chunks.empty()) return nullptr;

    int chunk_samples = detail;  // samples per chunk at the audio's native rate
    int total_samples = (int)chunks.size() * chunk_samples;
    std::vector<float> buffer(total_samples, 0.0f);

    for (int c = 0; c < (int)chunks.size(); c++) {
        auto& chunk = chunks[c];
        int top_n =(int)chunk.peaks.size();

        for (int i = 0; i < chunk_samples; i++) {
            float t = (float)i / data->sample_rate;  // local time within this chunk
            float sample = 0.0f;
            for (int p = 0; p < top_n; p++) {
                auto& peak = chunk.peaks[p];
                sample += glm::sin(2.0f * glm::pi<float>() * peak.freq * t + peak.phase) * peak.magnitude;
            }
            buffer[c * chunk_samples + i] = sample;
        }
    }

    // normalize
    float max_abs = 0.0f;
    for (float s : buffer) max_abs = std::max(max_abs, std::abs(s));
    if (max_abs > 1.0f)
        for (float& s : buffer) s /= max_abs;

    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(exinfo);
    exinfo.numchannels = 1;
    exinfo.defaultfrequency = data->sample_rate;
    exinfo.format = FMOD_SOUND_FORMAT_PCMFLOAT;
    exinfo.length = (unsigned int)(buffer.size() * sizeof(float));

    FMOD::Sound* sound = nullptr;
    system->createSound((const char*)buffer.data(), FMOD_OPENMEMORY | FMOD_OPENRAW | FMOD_CREATESAMPLE, &exinfo, &sound);
    return sound;
}

}  // namespace tmt
