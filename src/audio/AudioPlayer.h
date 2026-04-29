#pragma once

#include <vector>
#include <string>

class AudioPlayer
{
public:
    // Charge un fichier WAV. Reset la lecture, le player ne joue pas après chargement.
    bool loadFile(const std::string& path, int sampleRate);

    // Déclenche la lecture depuis le début.
    void start();

    // Remplit 'output' (stéréo entrelacé, 2 * numFrames floats) avec
    // les frames suivantes du buffer. Remplit de zéros si on ne joue pas.
    void process(float* output, int numFrames);

    // Indique si un fichier a été chargé avec succès.
    bool hasFile() const { return !buffer.empty(); }

private:
    std::vector<float> buffer;     // les échantillons décodés du WAV
    int channels = 0;              // 1 = mono, 2 = stéréo
    int readPosition = 0;          // position courante (en frames, pas en samples !)
    bool playing = false;          // est-ce qu'on est en train de jouer ?
};