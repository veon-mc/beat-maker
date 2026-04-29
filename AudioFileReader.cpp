#include "AudioFileReader.h"
#include <SDL3/SDL.h>
#include <iostream>

/*
 * Read a wav file and store the audio frames in the std::vector out_buffer.
 * Parameters :
 * - path : the path to the wav source file
 * - internalSampleRate : the sample-rate used by the audio engine (defined when PortAudio is initialized)
 * - out_buffer (output) : the buffer where the audio frames are stored
 * - out_channels (output) : number of audio channels (1 or 2) of out_buffer
*/
void AudioFileReader::loadFile(const std::string& path, int internalSampleRate, std::vector<float>& out_buffer, int& out_channels) {
    SDL_AudioSpec spec;
    Uint8* rawBuf = nullptr;
    Uint32 rawLen = 0;

    if (!SDL_LoadWAV(path.c_str(), &spec, &rawBuf, &rawLen)) {
        std::cerr << "Failed to load WAV file: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_AudioSpec dstSpec;
    dstSpec.format   = SDL_AUDIO_F32;
    dstSpec.channels = spec.channels;
    dstSpec.freq     = internalSampleRate;

    Uint8* floatBuf = nullptr;
    int floatLen = 0;

    if (!SDL_ConvertAudioSamples(&spec, rawBuf, static_cast<int>(rawLen), &dstSpec, &floatBuf, &floatLen)) {
        std::cerr << "Failed to convert audio samples: " << SDL_GetError() << std::endl;
        SDL_free(rawBuf);
        return;
    }

    SDL_free(rawBuf);

    int sampleCount = floatLen / static_cast<int>(sizeof(float));
    out_buffer.resize(sampleCount);
    float* floatData = reinterpret_cast<float*>(floatBuf);
    for (int i = 0; i < sampleCount; i++) {
        out_buffer[i] = floatData[i];
    }

    SDL_free(floatBuf);

    out_channels = spec.channels;
}
