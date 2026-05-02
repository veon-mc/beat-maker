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

    // Nouveau : volume et mute, pilotés par l'UI.
    void setVolume(float v) { volume.store(v); }
    float getVolume() const { return volume.load(); }

    void setMuted(bool m) { muted.store(m); }
    bool isMuted() const { return muted.load(); }
    std::string getFileName() const;
    void setReverse(bool r) { reverse.store(r); }
    bool isReverse() const { return reverse.load(); }
    std::string getFilePath() const { return filePath; }

private:
    std::mutex bufferMutex;
    std::vector<float> buffer;
    int channels = 0;
    int readPosition = 0;
    std::string filePath;
    std::atomic<bool> reverse{false};

    std::atomic<bool> playing{false};
    std::atomic<float> volume{1.0f};   // ← nouveau, valeur par défaut max
    std::atomic<bool> muted{false};    // ← nouveau
};