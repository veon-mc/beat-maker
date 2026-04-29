#include "Mixer.h"

void Mixer::mix(const float* source, float* destination, int numFrames, float volume)
{
    const int numSamples = numFrames * 2; // stéréo : 2 samples par frame
    for (int i = 0; i < numSamples; ++i) {
        destination[i] += source[i] * volume;
    }
}