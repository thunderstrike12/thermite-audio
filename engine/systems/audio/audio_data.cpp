#include "audio_data.hpp"
#include "engine/engine.hpp"
#include "fmod.hpp"

#include "engine/core/audio.hpp"

namespace tmt {

AudioData::AudioData(const IO::FileLocation& directory) : FileResource(directory) {}

bool AudioData::load() {
    const std::vector<char>& file_bytes = IO::read_file(file_location);

    FMOD::System* system = tmt::engine.audio.get_core_system();

    FMOD_CREATESOUNDEXINFO exinfo = {};
    exinfo.cbsize = sizeof(exinfo);
    exinfo.length = (unsigned int)file_bytes.size();

    FMOD::Sound* sound = nullptr;
    // FMOD_OPENMEMORY: bytes are compressed file data (mp3/wav container), let FMOD decode.
    // FMOD_CREATESAMPLE: force full decompression into memory so we can lock/read raw PCM after.
    FMOD_RESULT result = system->createSound(file_bytes.data(), FMOD_OPENMEMORY | FMOD_CREATESAMPLE, &exinfo, &sound);
    if (result != FMOD_OK) {
        Log::error("Failed to load audio file\n");
        return false;
    }

    FMOD_SOUND_TYPE type;
    FMOD_SOUND_FORMAT format;
    sound->getFormat(&type, &format, &channels, nullptr);

    float freq;
    sound->getDefaults(&freq, nullptr);
    sample_rate = (int)freq;

    unsigned int length_bytes;
    sound->getLength(&length_bytes, FMOD_TIMEUNIT_PCMBYTES);

    void* ptr1 = nullptr;
    void* ptr2 = nullptr;
    unsigned int len1 = 0, len2 = 0;
    sound->lock(0, length_bytes, &ptr1, &ptr2, &len1, &len2);

    int total_samples = (int)(len1 / sizeof(int16_t));
    int16_t* raw = (int16_t*)ptr1;

    samples.clear();
    samples.reserve(total_samples / (channels > 0 ? channels : 1));

    for (int i = 0; i < total_samples; i += channels) {
        float sum = 0.0f;
        for (int c = 0; c < channels; c++) sum += raw[i + c] / 32768.0f;
        samples.push_back(sum / (float)channels);
    }

    sound->unlock(ptr1, ptr2, len1, len2);
    sound->release();

    return true;
}

void AudioData::unload() {
    samples.clear();
    sample_rate = 0;
    channels = 0;
}

bool AudioData::reload() {
    unload();
    return load();
}

}  // namespace tmt