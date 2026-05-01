#include "AudioPlayer.h"
#include "AudioFileReader.h"

bool AudioPlayer::loadFile(const std::string& path, int sampleRate)
{
    // On stoppe la lecture avant de toucher au buffer.
    playing = false;

    std::vector<float> tempBuffer;
    int tempChannels = 0;
    AudioFileReader::loadFile(path, sampleRate, tempBuffer, tempChannels);

    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer = std::move(tempBuffer);
        channels = tempChannels;
        readPosition = 0;
    }

    return !buffer.empty();
}

void AudioPlayer::start()
{
    readPosition = 0;
    playing = true;
}

void AudioPlayer::process(float* output, int numFrames)
{
    std::unique_lock<std::mutex> lock(bufferMutex, std::try_to_lock);
    if (!lock.owns_lock() || !playing.load() || buffer.empty()) {
        for (int i = 0; i < numFrames * 2; ++i) output[i] = 0.0f;
        return;
    }

    const int totalFramesInBuffer = static_cast<int>(buffer.size()) / channels;

    for (int i = 0; i < numFrames; ++i) {
        if (readPosition >= totalFramesInBuffer) {
            output[i * 2] = 0.0f;
            output[i * 2 + 1] = 0.0f;
            playing = false;
            continue;
        }

        if (channels == 1) {
            float sample = buffer[readPosition];
            output[i * 2] = sample;
            output[i * 2 + 1] = sample;
        } else {
            output[i * 2] = buffer[readPosition * 2];
            output[i * 2 + 1] = buffer[readPosition * 2 + 1];
        }
        readPosition++;
    }
}

bool AudioPlayer::hasFile() const
{
    return !buffer.empty();
}