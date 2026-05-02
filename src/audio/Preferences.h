#pragma once

#include <array>
#include <string>

class Preferences
{
public:
    static constexpr int NUM_TRACKS = 4;

    // Charge les chemins depuis preferences.txt.
    // Retourne un tableau de 4 chaînes (vides si pas de fichier).
    static std::array<std::string, NUM_TRACKS> load(const std::string& filename = "preferences.txt");

    // Sauvegarde les 4 chemins dans preferences.txt.
    static void save(const std::array<std::string, NUM_TRACKS>& paths,
                     const std::string& filename = "preferences.txt");
};