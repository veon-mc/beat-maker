#include "Mixer.h"

void Mixer::mix(const float* source, float* destination, int numFrames, float volume)
{
    const int numSamples = numFrames * 2; // stereo: 2 samples per frame
    for (int i = 0; i < numSamples; ++i) {
        destination[i] += source[i] * volume;
    }
}