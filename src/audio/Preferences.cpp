#include "Preferences.h"
#include <fstream>

std::array<std::string, Preferences::NUM_TRACKS> Preferences::load(const std::string& filename)
{
    std::array<std::string, NUM_TRACKS> paths{};

    std::ifstream file(filename);
    if (!file.is_open()) {
        // No preferences file yet: return 4 empty paths.
        return paths;
    }

    for (int i = 0; i < NUM_TRACKS; ++i) {
        if (!std::getline(file, paths[i])) {
            // End of file reached early; remaining paths stay empty.
            break;
        }
    }

    return paths;
}

void Preferences::save(const std::array<std::string, NUM_TRACKS>& paths,
                       const std::string& filename)
{
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (int i = 0; i < NUM_TRACKS; ++i) {
        file << paths[i] << '\n';
    }
}