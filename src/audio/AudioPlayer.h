#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>

class AudioPlayer
{
public:
    bool loadFile(const std::string& path, int sampleRate);
    void start();
    void process(float* output, int numFrames);
    bool hasFile() const;

private:
    // Mutex protégeant le buffer et les channels (modifiés au chargement,
    // lus par le thread audio).
    std::mutex bufferMutex;
    std::vector<float> buffer;
    int channels = 0;

    // readPosition n'est touché QUE par le thread audio, pas besoin d'atomic.
    int readPosition = 0;

    // playing est écrit par l'UI (start()) et lu/écrit par l'audio (process()).
    // -> atomic.
    std::atomic<bool> playing{false};
};