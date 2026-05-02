#pragma once

#include "DrumMachine.h"
#include <SDL3/SDL.h>
#include <string>
#include <mutex>
#include <atomic>

/**
 * @class MainWindow
 * @brief Top-level Dear ImGui interface of the drum machine.
 *
 * Owns no audio state of its own; it reads from and writes to the
 * DrumMachine instance passed in the constructor. The SDL window is
 * required to host the native file dialog used by "Load sample".
 */
class MainWindow
{
public:
    /**
     * @brief Constructor.
     * @param drumMachine The audio engine the UI controls.
     * @param window The SDL window used as parent of the file dialog.
     */
    MainWindow(DrumMachine& drumMachine, SDL_Window* window);

    /**
     * @brief Draws the whole UI for the current frame.
     *
     * Must be called once per frame, between ImGui::NewFrame() and
     * ImGui::Render(). Also processes any pending file load that
     * was queued by the file dialog callback.
     */
    void draw();

private:
    /**
     * @brief Draws the top bar (Play/Stop button and BPM controls).
     */
    void drawTopBar();

    /**
     * @brief Draws the four-track grid with volume sliders and mute buttons.
     */
    void drawTracks();

    /**
     * @brief Draws the inspector panel for the currently selected track.
     */
    void drawInspector();

    /**
     * @brief Loads any sample queued by the file dialog callback.
     *
     * Called at the start of each draw() to bridge between the SDL
     * dialog (which may run on its own thread) and the main thread
     * that owns the audio engine.
     */
    void consumePendingFileLoad();

    /**
     * @brief Opens the native file dialog to pick a WAV for a track.
     * @param track Target track index for the loaded sample.
     */
    void openFileDialog(int track);

    /**
     * @brief Static SDL callback invoked when the user closes the dialog.
     *
     * Stores the chosen path and the target track index in the
     * shared state so consumePendingFileLoad() can pick them up.
     *
     * @param userdata Pointer to the MainWindow instance.
     * @param filelist Array of selected paths (NULL on cancel).
     * @param filter Index of the active file filter (unused).
     */
    static void SDLCALL fileDialogCallback(void* userdata,
                                           const char* const* filelist,
                                           int filter);

    DrumMachine& drumMachine;          ///< Audio engine driven by this UI.
    SDL_Window* sdlWindow;             ///< Parent window for the file dialog.

    int selectedTrack = 0;             ///< Track currently displayed in the inspector.

    std::mutex pendingFileMutex;       ///< Protects pendingFilePath against concurrent access.
    std::string pendingFilePath;       ///< Path picked by the user, waiting to be loaded.
    std::atomic<int> pendingFileTrack{-1}; ///< Track to load into (>=0), or -1 if nothing pending.
    int pendingFileTargetTrack = 0;    ///< Track captured when the dialog was opened.
};