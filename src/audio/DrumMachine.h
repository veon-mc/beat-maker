#pragma once

#include "AudioPlayer.h"
#include "Sequencer.h"
#include "Mixer.h"
#include "Delay.h"
#include <portaudio.h>
#include <string>
#include <array>

/**
 * @class DrumMachine
 * @brief Top-level audio engine of the application.
 *
 * Owns the four AudioPlayer / Delay pairs, the Sequencer and the Mixer,
 * and drives them through the PortAudio callback. Hides PortAudio
 * details behind a simple interface that the UI can use without
 * knowing anything about the audio implementation.
 *
 * The audio signal flow per track is:
 *   Sequencer -> AudioPlayer -> Delay -> Mixer -> output
 */
class DrumMachine
{
public:
    static constexpr int NUM_TRACKS = 4;     ///< Number of tracks.
    static constexpr int SAMPLE_RATE = 44100; ///< Sample rate used by the engine (Hz).
    static constexpr int BUFFER_SIZE = 256;   ///< Audio callback buffer size (frames).

    /**
     * @brief Default constructor. Does not initialize PortAudio yet.
     */
    DrumMachine();

    /**
     * @brief Destructor. Calls shutdown() to release PortAudio resources.
     */
    ~DrumMachine();

    /**
     * @brief Initializes PortAudio, the modules and starts the audio stream.
     *
     * Also loads the saved sample paths from preferences.txt if available.
     *
     * @return true on success, false if PortAudio initialization failed.
     */
    bool init();

    /**
     * @brief Stops the audio stream and releases PortAudio resources.
     *
     * Safe to call multiple times.
     */
    void shutdown();

    /**
     * @brief Returns a reference to the AudioPlayer of a given track.
     * @param track Track index (0 to NUM_TRACKS - 1).
     * @return Reference to the corresponding AudioPlayer.
     */
    AudioPlayer& getPlayer(int track) { return players[track]; }

    /**
     * @brief Returns a reference to the Delay of a given track.
     * @param track Track index (0 to NUM_TRACKS - 1).
     * @return Reference to the corresponding Delay.
     */
    Delay& getDelay(int track) { return delays[track]; }

    /**
     * @brief Returns a reference to the Sequencer.
     * @return Reference to the Sequencer.
     */
    Sequencer& getSequencer() { return sequencer; }

    /**
     * @brief Loads a WAV file on the given track and saves preferences.
     * @param track Track index (0 to NUM_TRACKS - 1).
     * @param path Path to the WAV file.
     * @return true on success, false if the track index is invalid or loading failed.
     */
    bool loadFile(int track, const std::string& path);

    /**
     * @brief Reads preferences.txt and loads the saved samples on each track.
     */
    void loadPreferences();

    /**
     * @brief Writes the currently loaded sample paths into preferences.txt.
     */
    void savePreferences() const;

private:
    /**
     * @brief Static trampoline used by PortAudio.
     *
     * PortAudio is a C library and only accepts free functions, not class
     * methods. This static function is registered as the callback, and it
     * forwards the call to the audioCallback() method through the
     * userData pointer.
     */
    static int audioCallbackStatic(const void* input, void* output,
                                   unsigned long framesPerBuffer,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags statusFlags,
                                   void* userData);

    /**
     * @brief Real audio callback method, called by audioCallbackStatic().
     *
     * Drives the sequencer, processes each player and delay, then mixes
     * everything into the output buffer.
     *
     * @param output Interleaved stereo output buffer (2 * numFrames floats).
     * @param numFrames Number of stereo frames to produce.
     * @return PortAudio status (paContinue to keep the stream running).
     */
    int audioCallback(float* output, int numFrames);

    std::array<AudioPlayer, NUM_TRACKS> players;   ///< Per-track sample players.
    std::array<Delay, NUM_TRACKS> delays;          ///< Per-track delay effects.
    Sequencer sequencer;                           ///< Step sequencer driving playback.
    Mixer mixer;                                   ///< Mixer that sums all tracks.

    PaStream* stream = nullptr;                    ///< PortAudio stream handle.

    float tempBuffer[BUFFER_SIZE * 2] = {0};       ///< Reusable per-track work buffer.
};