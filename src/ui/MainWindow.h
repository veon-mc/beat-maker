#pragma once

#include "DrumMachine.h"
#include <SDL3/SDL.h>
#include <string>
#include <mutex>
#include <atomic>

class MainWindow
{
public:
    // On garde une référence vers le moteur audio. La fenêtre SDL sert au file dialog.
    MainWindow(DrumMachine& drumMachine, SDL_Window* window);

    // Dessine toute l'UI. À appeler une fois par frame, entre NewFrame() et Render().
    void draw();

private:
    // Étapes du dessin, séparées pour la lisibilité.
    void drawTopBar();
    void drawTracks();
    void drawInspector();

    // Vérifie si un fichier a été sélectionné via le dialog et le charge.
    void consumePendingFileLoad();

    // Ouvre le file dialog pour la piste donnée.
    void openFileDialog(int track);

    // Callback statique appelée par SDL après sélection.
    static void SDLCALL fileDialogCallback(void* userdata,
                                           const char* const* filelist,
                                           int filter);

    DrumMachine& drumMachine;
    SDL_Window* sdlWindow;

    int selectedTrack = 0;

    // État du file dialog (partagé avec la callback statique).
    std::mutex pendingFileMutex;
    std::string pendingFilePath;
    std::atomic<int> pendingFileTrack{-1};
    int pendingFileTargetTrack = 0;
};