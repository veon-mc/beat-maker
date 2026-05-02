#pragma once

#include <vector>
#include <atomic>

/**
 * @class Delay
 * @brief Echo / delay audio effect using a circular ring buffer.
 *
 * Each call to process() reads samples from earlier in time and mixes
 * them into the current samples, producing an echo. The output is
 * also written back into the ring buffer, which creates feedback and
 * a natural decaying chain of echoes.
 *
 * Thread safety: parameters (enabled, time, mix) are std::atomic
 * because they are written by the UI thread and read by the audio
 * thread. The ring buffer itself is touched only by the audio thread.
 */
class Delay
{
public:
    /**
     * @brief Allocates the ring buffer for the maximum delay duration.
     *
     * Must be called once before the first process() call.
     *
     * @param sampleRate Audio sample rate (Hz).
     * @param maxDelaySeconds Maximum delay duration the buffer can hold (in seconds).
     */
    void init(int sampleRate, float maxDelaySeconds = 1.0f);

    /**
     * @brief Enables or disables the effect.
     * @param e true to enable, false to bypass.
     */
    void setEnabled(bool e) { enabled.store(e); }

    /**
     * @brief Indicates whether the effect is currently enabled.
     * @return true if enabled, false otherwise.
     */
    bool isEnabled() const  { return enabled.load(); }

    /**
     * @brief Sets the delay time.
     * @param t Delay duration in seconds (0.0 to 1.0).
     */
    void setTime(float t)   { time.store(t); }

    /**
     * @brief Returns the current delay time.
     * @return Delay duration in seconds.
     */
    float getTime() const   { return time.load(); }

    /**
     * @brief Sets the wet signal amplitude.
     * @param m Mix value (0.0 = no echo, 1.0 = full feedback).
     */
    void setMix(float m)    { mix.store(m); }

    /**
     * @brief Returns the current wet signal amplitude.
     * @return Mix value.
     */
    float getMix() const    { return mix.load(); }

    /**
     * @brief Applies the delay effect in place to a stereo buffer.
     *
     * If the effect is disabled, the buffer is left unchanged.
     *
     * @param buffer Interleaved stereo buffer (2 * numFrames floats).
     * @param numFrames Number of stereo frames to process.
     */
    void process(float* buffer, int numFrames);

private:
    int sampleRate = 44100;          ///< Audio sample rate.
    int bufferSize = 0;              ///< Ring buffer size in frames.

    std::vector<float> bufferL;      ///< Left channel ring buffer.
    std::vector<float> bufferR;      ///< Right channel ring buffer.
    int writeIndex = 0;              ///< Next position to write into the buffers.

    std::atomic<bool>  enabled{false};   ///< Effect enabled flag (UI -> audio).
    std::atomic<float> time{0.0f};       ///< Delay time in seconds (UI -> audio).
    std::atomic<float> mix{0.0f};        ///< Wet signal amplitude (UI -> audio).
};