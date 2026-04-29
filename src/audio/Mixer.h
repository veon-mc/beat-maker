#pragma once

class Mixer
{
public:
    // Additionne 'numFrames' frames stéréo (donc 2 * numFrames floats)
    // depuis 'source' dans 'destination'.
    // 'volume' multiplie le signal source avant addition.
    void mix(const float* source, float* destination, int numFrames, float volume);
};