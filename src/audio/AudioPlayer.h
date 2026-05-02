#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>

/**
 * @class AudioPlayer
 * @brief Plays a single audio sample loaded from a WAV file.
 *
 * Holds the decoded sample buffer (mono or stereo) and a read position.
 * When start() is called, playback begins from the start (or from the
 * end if reverse mode is enabled). Each process() call outputs the
 * next slice of the sample, or silence when not playing.
 *
 * Thread safety: the sample buffer is protected by a mutex and is
 * written only on file load (UI thread) and read only on playback
 * (audio thread). Per-track parameters (volume, mute, reverse) are
 * std::atomic. The audio thread uses try_to_lock to never block.
 */
class AudioPlayer
{
public:
    /**
     * @brief Loads a WAV file into the player buffer.
     *
     * Stops playback before swapping the buffer to ensure a clean transition.
     *
     * @param path Path to the WAV file.
     * @param sampleRate Target sample rate (typically 44100).
     * @return true on success, false if the file could not be loaded.
     */
    bool loadFile(const std::string& path, int sampleRate);

    /**
     * @brief Triggers playback from the start (or end, in reverse mode).
     */
    void start();

    /**
     * @brief Renders the next slice of audio into the output buffer.
     *
     * Outputs zeros if the player is idle, has no file, or if the buffer
     * mutex is currently held by another thread.
     *
     * @param output Interleaved stereo output buffer (2 * numFrames floats).
     * @param numFrames Number of stereo frames to produce.
     */
    void process(float* output, int numFrames);

    /**
     * @brief Indicates whether a sample has been loaded.
     * @return true if a sample is loaded, false otherwise.
     */
    bool hasFile() const;

    /**
     * @brief Sets the playback volume.
     * @param v Volume (0.0 = silent, 1.0 = unchanged).
     */
    void setVolume(float v) { volume.store(v); }

    /**
     * @brief Returns the current playback volume.
     * @return Volume value (0.0 to 1.0).
     */
    float getVolume() const { return volume.load(); }

    /**
     * @brief Mutes or unmutes the track.
     * @param m true to mute, false to unmute.
     */
    void setMuted(bool m) { muted.store(m); }

    /**
     * @brief Indicates whether the track is muted.
     * @return true if muted.
     */
    bool isMuted() const  { return muted.load(); }

    /**
     * @brief Enables or disables reverse playback.
     * @param r true to play backwards, false for normal playback.
     */
    void setReverse(bool r) { reverse.store(r); }

    /**
     * @brief Indicates whether reverse playback is enabled.
     * @return true if reverse is on.
     */
    bool isReverse() const  { return reverse.load(); }

    /**
     * @brief Returns just the file name of the loaded sample (no directory).
     * @return Bare file name, or empty if no file is loaded.
     */
    std::string getFileName() const;

    /**
     * @brief Returns the full path of the loaded sample.
     * @return The full file path, or empty if no file is loaded.
     */
    std::string getFilePath() const { return filePath; }

private:
    std::mutex bufferMutex;            ///< Protects buffer/channels/filePath against concurrent access.
    std::vector<float> buffer;         ///< Decoded sample data (mono or interleaved stereo).
    int channels = 0;                  ///< 1 = mono, 2 = stereo.
    std::string filePath;              ///< Full path of the currently loaded sample.

    int readPosition = 0;              ///< Current playback position, in frames (audio thread only).

    std::atomic<bool>  playing{false}; ///< Playback active flag.
    std::atomic<float> volume{1.0f};   ///< Track volume (UI -> audio).
    std::atomic<bool>  muted{false};   ///< Mute flag (UI -> audio).
    std::atomic<bool>  reverse{false}; ///< Reverse playback flag (UI -> audio).
};