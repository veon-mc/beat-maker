#include "AudioPlayer.h"
#include "AudioFileReader.h"

bool AudioPlayer::loadFile(const std::string& path, int sampleRate)
{
    // Stop playback before touching the buffer.
    playing = false;

    std::vector<float> tempBuffer;
    int tempChannels = 0;
    AudioFileReader::loadFile(path, sampleRate, tempBuffer, tempChannels);

    // Critical section: swap the buffer while the audio thread is outside
    // process(). std::lock_guard locks on construction and unlocks
    // automatically when leaving the scope (RAII).
    {
        std::lock_guard<std::mutex> lock(bufferMutex);
        buffer = std::move(tempBuffer);
        channels = tempChannels;
        readPosition = 0;
        filePath = path;
    }

    return !buffer.empty();
}

void AudioPlayer::start()
{
    // In normal mode start from the beginning, in reverse mode start from the end.
    if (reverse.load()) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        if (!buffer.empty() && channels > 0) {
            const int totalFrames = static_cast<int>(buffer.size()) / channels;
            readPosition = totalFrames - 1;
        } else {
            readPosition = 0;
        }
    } else {
        readPosition = 0;
    }
    playing = true;
}

void AudioPlayer::process(float* output, int numFrames)
{
    // try_lock: if we cannot acquire the mutex (e.g. UI is loading a file),
    // output silence and return. Never block the audio thread on a mutex.
    std::unique_lock<std::mutex> lock(bufferMutex, std::try_to_lock);
    if (!lock.owns_lock() || !playing.load() || buffer.empty()) {
        for (int i = 0; i < numFrames * 2; ++i) output[i] = 0.0f;
        return;
    }

    const int totalFrames = static_cast<int>(buffer.size()) / channels;
    const bool isReverse = reverse.load();

    for (int i = 0; i < numFrames; ++i) {
        // Bounds check depending on playback direction.
        const bool atEnd = isReverse ? (readPosition < 0) : (readPosition >= totalFrames);
        if (atEnd) {
            output[i * 2] = 0.0f;
            output[i * 2 + 1] = 0.0f;
            playing = false;
            continue;
        }

        if (channels == 1) {
            // Mono: duplicate the same sample on L and R.
            float sample = buffer[readPosition];
            output[i * 2]     = sample;
            output[i * 2 + 1] = sample;
        } else {
            // Interleaved stereo: L sample at 2*pos, R sample at 2*pos+1.
            output[i * 2]     = buffer[readPosition * 2];
            output[i * 2 + 1] = buffer[readPosition * 2 + 1];
        }

        // Advance in the chosen direction.
        if (isReverse) readPosition--;
        else           readPosition++;
    }
}

bool AudioPlayer::hasFile() const
{
    // Read-only access. Called from the UI thread; loadFile may modify it,
    // but in practice both calls happen on the same UI thread, so no race.
    return !buffer.empty();
}

std::string AudioPlayer::getFileName() const
{
    // Extract just the file name (without the full path).
    size_t pos = filePath.find_last_of("/\\");
    if (pos == std::string::npos) return filePath;
    return filePath.substr(pos + 1);
}