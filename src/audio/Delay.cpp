#include "Delay.h"

void Delay::init(int sampleRate_, float maxDelaySeconds)
{
    sampleRate = sampleRate_;
    bufferSize = static_cast<int>(static_cast<float>(sampleRate) * maxDelaySeconds);
    bufferL.assign(bufferSize, 0.0f);
    bufferR.assign(bufferSize, 0.0f);
    writeIndex = 0;
}

void Delay::process(float* buffer, int numFrames)
{
    // If disabled or not initialized, leave the buffer untouched.
    if (!enabled.load() || bufferSize == 0) return;

    const float currentMix  = mix.load();
    const float currentTime = time.load();

    // Distance between read and write indexes, in samples.
    int delaySamples = static_cast<int>(currentTime * static_cast<float>(sampleRate));

    // Safety: clamp between 1 and bufferSize - 1 to avoid feedback loop at 0.
    if (delaySamples < 1) delaySamples = 1;
    if (delaySamples >= bufferSize) delaySamples = bufferSize - 1;

    for (int i = 0; i < numFrames; ++i) {
        // readIndex = writeIndex - delaySamples, modulo bufferSize.
        // In C++, modulo of a negative value can be negative, so we add bufferSize.
        int readIndex = (writeIndex - delaySamples + bufferSize) % bufferSize;

        // Current input samples.
        float inL = buffer[i * 2];
        float inR = buffer[i * 2 + 1];

        // out = in + mix * buffer[readIndex]
        float outL = inL + currentMix * bufferL[readIndex];
        float outR = inR + currentMix * bufferR[readIndex];

        // Write the output back into the ring buffer (creates feedback).
        bufferL[writeIndex] = outL;
        bufferR[writeIndex] = outR;

        // Advance writeIndex circularly.
        writeIndex = (writeIndex + 1) % bufferSize;

        // Replace the buffer's samples with the wet output.
        buffer[i * 2]     = outL;
        buffer[i * 2 + 1] = outR;
    }
}