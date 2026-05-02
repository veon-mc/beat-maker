#pragma once

#include "AudioPlayer.h"
#include "Sequencer.h"
#include "Mixer.h"
#include "Delay.h"
#include <portaudio.h>
#include <string>
#include <array>

class DrumMachine
{
public:
    static constexpr int NUM_TRACKS = 4;
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BUFFER_SIZE = 256;

    DrumMachine();
    ~DrumMachine();

    // Initialise PortAudio et démarre le flux audio.
    bool init();

    // Stoppe et libère PortAudio.
    void shutdown();

    // --- Accès aux modules pour l'UI ---
    AudioPlayer& getPlayer(int track)   { return players[track]; }
    Delay&       getDelay(int track)    { return delays[track]; }
    Sequencer&   getSequencer()         { return sequencer; }

    // --- Helpers de haut niveau pour l'UI ---
    bool loadFile(int track, const std::string& path);

    // --- Sauvegarde / chargement des préférences ---
    void loadPreferences();
    void savePreferences();

private:
    // La callback statique appelée par PortAudio.
    // userData contiendra un pointeur vers l'instance DrumMachine.
    static int audioCallbackStatic(const void* input, void* output,
                                   unsigned long framesPerBuffer,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags statusFlags,
                                   void* userData);

    // La vraie callback, méthode d'instance (appelée depuis la statique).
    int audioCallback(float* output, int numFrames);

    std::array<AudioPlayer, NUM_TRACKS> players;
    std::array<Delay, NUM_TRACKS> delays;
    Sequencer sequencer;
    Mixer mixer;

    PaStream* stream = nullptr;

    // Buffer temporaire pour le mix piste par piste, alloué une fois.
    float tempBuffer[BUFFER_SIZE * 2] = {0};
};