#ifndef DRUM_MACHINE_AUDIOFILEREADER_H
#define DRUM_MACHINE_AUDIOFILEREADER_H

#include <vector>
#include <string>

class AudioFileReader {
public:
    static void loadFile(const std::string &path, int internalSampleRate, std::vector<float> &out_buffer, int &out_channels);
};

#endif //DRUM_MACHINE_AUDIOFILEREADER_H
