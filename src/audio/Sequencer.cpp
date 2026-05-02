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
    // Full reset on each play.
    currentBeat = 0.0;
    lastStepTriggered = -1;
    currentStep.store(0);
    playing.store(true);
}

void Sequencer::stop()
{
    playing.store(false);
    currentStep.store(0); // reset playhead to step 0 (see spec section 4.1.2)
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

    // How many beats does this buffer represent?
    const double beatsPerBuffer = static_cast<double>(numFrames) * bpm.load() / (60.0 * sampleRate);

    // Advance musical time.
    currentBeat += beatsPerBuffer;

    // One step = 1/4 beat. 16 steps = 4 beats. Loop modulo 4 beats.
    if (currentBeat >= 4.0) currentBeat -= 4.0;

    // Which step should be active right now?
    const int step = static_cast<int>(currentBeat / 0.25) % NUM_STEPS;

    // If we just crossed a step boundary, trigger the matching sounds.
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