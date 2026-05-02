#pragma once

#include <array>
#include <string>

/**
 * @class Preferences
 * @brief Persists the per-track sample paths to a plain text file.
 *
 * The preferences file stores one path per line, one line per track,
 * in track order. Empty lines mean the corresponding track has no
 * loaded sample. The file is written in the working directory of the
 * executable.
 */
class Preferences
{
public:
    static constexpr int NUM_TRACKS = 4; ///< Expected number of tracks.

    /**
     * @brief Loads sample paths from a preferences file.
     *
     * If the file does not exist or cannot be opened, returns an array
     * of empty strings.
     *
     * @param filename Path to the preferences file (default: "preferences.txt").
     * @return Array of NUM_TRACKS strings (empty for missing or unset tracks).
     */
    static std::array<std::string, NUM_TRACKS> load(const std::string& filename = "preferences.txt");

    /**
     * @brief Saves sample paths into a preferences file.
     *
     * Each path is written on its own line, in track order.
     *
     * @param paths Sample paths to save.
     * @param filename Path to the preferences file (default: "preferences.txt").
     */
    static void save(const std::array<std::string, NUM_TRACKS>& paths,
                     const std::string& filename = "preferences.txt");
};