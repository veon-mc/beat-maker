#include "Delay.h"

void Delay::init(int sampleRate_, float maxDelaySeconds)
{
    sampleRate = sampleRate_;
    bufferSize = static_cast<int>(sampleRate * maxDelaySeconds);
    bufferL.assign(bufferSize, 0.0f);
    bufferR.assign(bufferSize, 0.0f);
    writeIndex = 0;
}

void Delay::process(float* buffer, int numFrames)
{
    // Si désactivé ou non initialisé, on ne touche à rien.
    if (!enabled.load() || bufferSize == 0) return;

    const float currentMix  = mix.load();
    const float currentTime = time.load();

    // Distance entre read et write, en samples.
    int delaySamples = static_cast<int>(currentTime * sampleRate);

    // Sécurité : on borne entre 1 et bufferSize-1.
    if (delaySamples < 1) delaySamples = 1;
    if (delaySamples >= bufferSize) delaySamples = bufferSize - 1;

    for (int i = 0; i < numFrames; ++i) {
        // readIndex = writeIndex - delaySamples, modulo bufferSize.
        // En C++, le modulo d'un négatif peut donner un négatif → on ajoute bufferSize.
        int readIndex = (writeIndex - delaySamples + bufferSize) % bufferSize;

        // Sample courant (entrée).
        float inL = buffer[i * 2];
        float inR = buffer[i * 2 + 1];

        // out = in + mix * buffer[readIndex]
        float outL = inL + currentMix * bufferL[readIndex];
        float outR = inR + currentMix * bufferR[readIndex];

        // Écrit la sortie dans le ring buffer (feedback).
        bufferL[writeIndex] = outL;
        bufferR[writeIndex] = outR;

        // Avance writeIndex circulairement.
        writeIndex = (writeIndex + 1) % bufferSize;

        // Remplace les samples du buffer de sortie.
        buffer[i * 2]     = outL;
        buffer[i * 2 + 1] = outR;
    }
}