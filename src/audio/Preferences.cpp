#include "Preferences.h"
#include <fstream>

std::array<std::string, Preferences::NUM_TRACKS> Preferences::load(const std::string& filename)
{
    std::array<std::string, NUM_TRACKS> paths{};

    std::ifstream file(filename);
    if (!file.is_open()) {
        // Pas de fichier de préférences : on retourne 4 chemins vides.
        return paths;
    }

    for (int i = 0; i < NUM_TRACKS; ++i) {
        if (!std::getline(file, paths[i])) {
            // Fin de fichier prématurée, on laisse les chemins suivants vides.
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