#pragma once

#include <vector>
#include <atomic>

class Delay
{
public:
    // Initialise le ring buffer pour une durée max (en secondes) à un sample rate donné.
    void init(int sampleRate, float maxDelaySeconds = 1.0f);

    // Active / désactive l'effet (depuis l'UI).
    void setEnabled(bool e) { enabled.store(e); }
    bool isEnabled() const { return enabled.load(); }

    // Temps du delay en secondes (0 à 1).
    void setTime(float t)  { time.store(t); }
    float getTime() const  { return time.load(); }

    // Intensité du signal retardé (0 à 1).
    void setMix(float m)   { mix.store(m); }
    float getMix() const   { return mix.load(); }

    // Applique l'effet sur un buffer stéréo entrelacé.
    void process(float* buffer, int numFrames);

private:
    int sampleRate = 44100;
    int bufferSize = 0;

    std::vector<float> bufferL;  // ring buffer canal gauche
    std::vector<float> bufferR;  // ring buffer canal droit
    int writeIndex = 0;

    std::atomic<bool>  enabled{false};
    std::atomic<float> time{0.0f};   // en secondes
    std::atomic<float> mix{0.0f};    // 0 à 1
};