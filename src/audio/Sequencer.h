#pragma once

#include <atomic>
#include <array>

class AudioPlayer;

/**
 * @class Sequencer
 * @brief Step sequencer driving the playback grid.
 *
 * Maintains a 4 tracks x 16 steps grid of on/off booleans, plus the
 * current playing position. Time advances on the audio thread (via
 * process()), based on the buffer size and the configured BPM. When
 * the playhead crosses a step boundary, the active tracks for that
 * step trigger their assigned AudioPlayer.
 *
 * Thread safety: parameters writable by the UI (BPM, steps, play
 * state) are stored as std::atomic. The current step is computed by
 * the audio thread and read by the UI to draw the playhead.
 */
class Sequencer
{
public:
    static constexpr int NUM_TRACKS = 4;  ///< Number of tracks in the sequencer.
    static constexpr int NUM_STEPS = 16;  ///< Number of steps per track.

    /**
     * @brief Registers the AudioPlayer to trigger for a given track.
     *
     * Called once at startup to wire each track to its corresponding player.
     *
     * @param trackIndex Track index (0 to NUM_TRACKS - 1).
     * @param player Pointer to the AudioPlayer to associate with this track.
     */
    void setPlayer(int trackIndex, AudioPlayer* player);

    /**
     * @brief Starts sequencer playback from step 0.
     *
     * Resets the current step and the internal beat counter.
     */
    void start();

    /**
     * @brief Stops sequencer playback and resets the playhead to step 0.
     */
    void stop();

    /**
     * @brief Indicates whether the sequencer is currently running.
     * @return true if playing, false otherwise.
     */
    bool isPlaying() const { return playing.load(); }

    /**
     * @brief Activates or deactivates a step on a given track.
     * @param track Track index (0 to NUM_TRACKS - 1).
     * @param step Step index (0 to NUM_STEPS - 1).
     * @param active true to activate, false to deactivate.
     */
    void setStep(int track, int step, bool active);

    /**
     * @brief Reads the on/off state of a step.
     * @param track Track index.
     * @param step Step index.
     * @return true if the step is active, false otherwise.
     */
    bool getStep(int track, int step) const;

    /**
     * @brief Sets the playback tempo in beats per minute.
     * @param bpm New tempo value.
     */
    void setBpm(float bpm) { this->bpm.store(bpm); }

    /**
     * @brief Returns the current tempo.
     * @return The BPM value.
     */
    float getBpm() const { return bpm.load(); }

    /**
     * @brief Returns the step currently being played.
     *
     * Used by the UI to render the playhead. Computed on the audio thread.
     *
     * @return The index of the current step (0 to NUM_STEPS - 1).
     */
    int getCurrentStep() const { return currentStep.load(); }

    /**
     * @brief Advances the sequencer by one audio buffer.
     *
     * Called once per audio callback. Computes the elapsed musical time
     * from the buffer size and triggers the players when the step changes.
     *
     * @param numFrames Number of frames in the audio buffer.
     * @param sampleRate Audio sample rate (Hz).
     */
    void process(int numFrames, int sampleRate);

private:
    std::array<AudioPlayer*, NUM_TRACKS> players{};                              ///< Players to trigger.
    std::array<std::array<std::atomic<bool>, NUM_STEPS>, NUM_TRACKS> grid{};     ///< 4 x 16 step grid.

    std::atomic<bool>  playing{false};        ///< Sequencer running flag.
    std::atomic<float> bpm{120.0f};           ///< Tempo in beats per minute.
    std::atomic<int>   currentStep{0};        ///< Step currently played (audio -> UI).

    double currentBeat = 0.0;                 ///< Current position in beats (audio thread only).
    int    lastStepTriggered = -1;            ///< Last triggered step (audio thread only).
};