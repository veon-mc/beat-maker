#include "Sequencer.h"
#include "AudioPlayer.h"

void Sequencer::setPlayer(int trackIndex, AudioPlayer* player)
{
    if (trackIndex >= 0 && trackIndex < NUM_TRACKS) {
        players[trackIndex] = player;
    }
}

void Sequencer::start()
{
    // Reset complet à chaque play.
    currentBeat = 0.0;
    lastStepTriggered = -1;
    currentStep.store(0);
    playing.store(true);
}

void Sequencer::stop()
{
    playing.store(false);
    currentStep.store(0); 
}

void Sequencer::setStep(int track, int step, bool active)
{
    if (track >= 0 && track < NUM_TRACKS && step >= 0 && step < NUM_STEPS) {
        grid[track][step].store(active);
    }
}

bool Sequencer::getStep(int track, int step) const
{
    if (track >= 0 && track < NUM_TRACKS && step >= 0 && step < NUM_STEPS) {
        return grid[track][step].load();
    }
    return false;
}

void Sequencer::process(int numFrames, int sampleRate)
{
    if (!playing.load()) return;

    // Combien de beats représente ce buffer ?
    const double beatsPerBuffer = (double)numFrames * bpm.load() / (60.0 * sampleRate);

    // On avance dans le temps musical.
    currentBeat += beatsPerBuffer;

    // Un pas = 1/4 de beat. 16 pas = 4 beats. On boucle modulo 4 beats.
    if (currentBeat >= 4.0) currentBeat -= 4.0;

    const int step = static_cast<int>(currentBeat / 0.25) % NUM_STEPS;

    // Si on vient de changer de pas, on déclenche les sons.
    if (step != lastStepTriggered) {
        for (int track = 0; track < NUM_TRACKS; ++track) {
            if (grid[track][step].load() && players[track] != nullptr) {
                players[track]->start();
            }
        }
        lastStepTriggered = step;
        currentStep.store(step);
    }
}