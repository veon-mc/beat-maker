#include "DrumMachine.h"
#include "Preferences.h"
#include <cstdio>

DrumMachine::DrumMachine() = default;

DrumMachine::~DrumMachine()
{
    shutdown();
}

bool DrumMachine::init()
{
    // 1. Init PortAudio.
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("Pa_Initialize: %s\n", Pa_GetErrorText(err));
        return false;
    }

    // 2. Init each delay's ring buffer.
    for (int t = 0; t < NUM_TRACKS; ++t) {
        delays[t].init(SAMPLE_RATE, 1.0f);
    }

    // 3. Wire players to the sequencer.
    for (int t = 0; t < NUM_TRACKS; ++t) {
        sequencer.setPlayer(t, &players[t]);
    }

    // 4. Load saved preferences (auto-loads sample paths from previous session).
    loadPreferences();

    // 5. Open the audio stream, passing 'this' as userData.
    err = Pa_OpenDefaultStream(
        &stream, 0, 2, paFloat32,
        SAMPLE_RATE, BUFFER_SIZE,
        &DrumMachine::audioCallbackStatic,
        this
    );
    if (err != paNoError) {
        printf("Pa_OpenDefaultStream: %s\n", Pa_GetErrorText(err));
        return false;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Pa_StartStream: %s\n", Pa_GetErrorText(err));
        return false;
    }

    return true;
}

void DrumMachine::shutdown()
{
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
        Pa_Terminate();
    }
}

int DrumMachine::audioCallbackStatic(const void*, void* output,
                                     unsigned long framesPerBuffer,
                                     const PaStreamCallbackTimeInfo*,
                                     PaStreamCallbackFlags, void* userData)
{
    auto* instance = static_cast<DrumMachine*>(userData);
    return instance->audioCallback(static_cast<float*>(output),
                                   static_cast<int>(framesPerBuffer));
}

int DrumMachine::audioCallback(float* output, int numFrames)
{
    // 1. Advance the sequencer (may trigger players).
    sequencer.process(numFrames, SAMPLE_RATE);

    // 2. Reset the output buffer to silence.
    for (int i = 0; i < numFrames * 2; ++i) output[i] = 0.0f;

    // 3. For each track: process player -> delay -> mix into output.
    for (int t = 0; t < NUM_TRACKS; ++t) {
        players[t].process(tempBuffer, numFrames);
        delays[t].process(tempBuffer, numFrames);

        float volume = players[t].isMuted() ? 0.0f : players[t].getVolume();
        mixer.mix(tempBuffer, output, numFrames, volume);
    }

    return paContinue;
}

bool DrumMachine::loadFile(int track, const std::string& path)
{
    if (track < 0 || track >= NUM_TRACKS) return false;
    bool ok = players[track].loadFile(path, SAMPLE_RATE);
    if (ok) savePreferences();
    return ok;
}

void DrumMachine::loadPreferences()
{
    auto paths = Preferences::load();
    for (int t = 0; t < NUM_TRACKS; ++t) {
        if (!paths[t].empty()) {
            players[t].loadFile(paths[t], SAMPLE_RATE);
        }
    }
}

void DrumMachine::savePreferences() const
{
    std::array<std::string, NUM_TRACKS> paths;
    for (int t = 0; t < NUM_TRACKS; ++t) {
        paths[t] = players[t].getFilePath();
    }
    Preferences::save(paths);
}