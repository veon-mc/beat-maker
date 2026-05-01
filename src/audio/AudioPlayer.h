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
    std::mutex bufferMutex;
    std::vector<float> buffer;
    int channels = 0;

    int readPosition = 0;

    std::atomic<bool> playing{false};
};