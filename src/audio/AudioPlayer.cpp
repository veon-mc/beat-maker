#include "AudioPlayer.h"
#include "AudioFileReader.h"

bool AudioPlayer::loadFile(const std::string& path, int sampleRate)
{
    buffer.clear();
    channels = 0;
    readPosition = 0;
    playing = false;

    AudioFileReader::loadFile(path, sampleRate, buffer, channels);

    // Si le chargement a échoué (buffer vide), on signale.
    return !buffer.empty();
}

void AudioPlayer::start()
{
    readPosition = 0;
    playing = true;
}

void AudioPlayer::process(float* output, int numFrames)
{
    // Cas 1 : on ne joue pas → remplir de zéros et sortir.
    if (!playing || buffer.empty()) {
        for (int i = 0; i < numFrames * 2; ++i) {
            output[i] = 0.0f;
        }
        return;
    }

    // Combien de frames sont disponibles dans le buffer source ?
    const int totalFramesInBuffer = static_cast<int>(buffer.size()) / channels;

    for (int i = 0; i < numFrames; ++i) {
        // Si on a dépassé la fin du sample, on remplit de zéros et on s'arrête de jouer.
        if (readPosition >= totalFramesInBuffer) {
            output[i * 2]     = 0.0f;
            output[i * 2 + 1] = 0.0f;
            playing = false;
            continue;
        }

        // Sinon, on lit selon le nombre de canaux.
        if (channels == 1) {
            // Mono : on duplique le même sample sur L et R.
            float sample = buffer[readPosition];
            output[i * 2]     = sample;
            output[i * 2 + 1] = sample;
        } else {
            // Stéréo entrelacé : sample L à 2*pos, sample R à 2*pos+1.
            output[i * 2]     = buffer[readPosition * 2];
            output[i * 2 + 1] = buffer[readPosition * 2 + 1];
        }

        readPosition++;
    }
}