#pragma once

#include <atomic>
#include <array>

class AudioPlayer;

class Sequencer
{
public:
    static constexpr int NUM_TRACKS = 4;
    static constexpr int NUM_STEPS = 16;

    // Les AudioPlayers à déclencher (un par piste). On les enregistre
    // une fois pour toutes au démarrage.
    void setPlayer(int trackIndex, AudioPlayer* player);

    // Démarre / arrête le séquenceur (depuis l'UI).
    void start();
    void stop();
    bool isPlaying() const { return playing.load(); }

    // Active / désactive un pas (depuis l'UI).
    void setStep(int track, int step, bool active);
    bool getStep(int track, int step) const;

    // Réglage du BPM (depuis l'UI).
    void setBpm(float bpm) { this->bpm.store(bpm); }
    float getBpm() const { return bpm.load(); }

    // Pour l'UI : connaitre le pas en cours d'exécution.
    int getCurrentStep() const { return currentStep.load(); }

    // Appelé à chaque callback audio.
    void process(int numFrames, int sampleRate);

private:
    std::array<AudioPlayer*, NUM_TRACKS> players{};

    // Grille 4 x 16. atomic<bool> car écrite par l'UI, lue par l'audio.
    std::array<std::array<std::atomic<bool>, NUM_STEPS>, NUM_TRACKS> grid{};

    std::atomic<bool> playing{false};
    std::atomic<float> bpm{120.0f};

    std::atomic<int> currentStep{0};

    double currentBeat = 0.0;
    int lastStepTriggered = -1; // pour détecter le changement de pas
};