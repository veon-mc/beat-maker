#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <portaudio.h>
#include <cstdio>
#include <cmath>
#include "AudioFileReader.h"
#include <vector>
#include "AudioPlayer.h"
#include "Sequencer.h"
#include "Mixer.h"
#include <mutex>
#include <atomic>

static AudioPlayer players[4];
static Sequencer sequencer;
static constexpr int SAMPLE_RATE = 44100;
static Mixer mixer;
// État partagé pour la boîte de dialogue de fichier.
static std::mutex pendingFileMutex;
static std::string pendingFilePath;       // chemin sélectionné, vide si rien à charger
static std::atomic<int> pendingFileTrack{-1}; // sur quelle piste charger (-1 = rien)

// Callback appelée par SDL quand l'utilisateur a choisi un fichier (ou annulé).
static void SDLCALL fileDialogCallback(void* userdata, const char* const* filelist, int /*filter*/)
{
    // Si l'utilisateur a annulé, filelist est nullptr (ou son premier élément).
    if (filelist == nullptr || filelist[0] == nullptr) {
        return;
    }

    // L'index de la piste cible a été passé via userdata.
    const int track = static_cast<int>(reinterpret_cast<intptr_t>(userdata));

    // On stocke le chemin pour que le main thread le charge plus tard.
    {
        std::lock_guard<std::mutex> lock(pendingFileMutex);
        pendingFilePath = filelist[0];
    }
    pendingFileTrack.store(track);
}

static int audioCallback(const void*, void* output,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo*,
                         PaStreamCallbackFlags, void*)
{
    float* out = static_cast<float*>(output);
    const int numFrames = static_cast<int>(framesPerBuffer);

    // 1. Avance le sequencer.
    sequencer.process(numFrames, SAMPLE_RATE);

    // 2. Reset du buffer de sortie (silence).
    for (int i = 0; i < numFrames * 2; ++i) out[i] = 0.0f;

    // 3. Pour chaque piste : process le player, puis mix dans la sortie.
    static float tempBuffer[256 * 2];
    for (int t = 0; t < 4; ++t) {
        players[t].process(tempBuffer, numFrames);

        // Volume effectif = 0 si muté, sinon le volume de la piste.
        float effectiveVolume = players[t].isMuted() ? 0.0f : players[t].getVolume();
        mixer.mix(tempBuffer, out, numFrames, effectiveVolume);
    }

    return paContinue;
}

int main(int, char**)
{
    // ---- Init SDL + fenêtre + renderer ----
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Erreur SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Drum Machine", 1280, 250, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, 1100, 220);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    // ---- Init ImGui ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // ---- Init PortAudio ----
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        printf("Erreur Pa_Initialize: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Câble le player 0 au sequencer.
    for (int t = 0; t < 4; ++t) sequencer.setPlayer(t, &players[t]);

    PaStream* stream = nullptr;
    err = Pa_OpenDefaultStream(
        &stream,
        0,            // pas de canaux d'entrée
        2,            // 2 canaux de sortie (stéréo)
        paFloat32,    // format des échantillons : float
        44100,        // fréquence d'échantillonnage
        256,          // taille du buffer en frames
        audioCallback,
        nullptr       // données utilisateur (on en mettra plus tard)
    );
    if (err != paNoError) {
        printf("Erreur Pa_OpenDefaultStream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("Erreur Pa_StartStream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // ---- Boucle principale ----
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        // Si un fichier a été sélectionné via le dialog, on le charge maintenant.
        {
            const int targetTrack = pendingFileTrack.load();
            if (targetTrack >= 0) {
                std::string path;
                {
                    std::lock_guard<std::mutex> lock(pendingFileMutex);
                    path = pendingFilePath;
                    pendingFilePath.clear();
                }
                if (!path.empty()) {
                    players[targetTrack].loadFile(path, SAMPLE_RATE);
                }
                pendingFileTrack.store(-1);
            }
        }
        static int selectedTrack = 0;

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGuiWindowFlags windowFlags =
              ImGuiWindowFlags_NoTitleBar      // pas de barre de titre
            | ImGuiWindowFlags_NoResize        // pas de redimensionnement
            | ImGuiWindowFlags_NoMove          // pas déplaçable
            | ImGuiWindowFlags_NoCollapse      // pas de bouton "réduire"
            | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::Begin("Drum Machine", nullptr, windowFlags);

        // --- Ligne du haut : Play / BPM (au-dessus, sur toute la largeur) ---
        if (sequencer.isPlaying()) {
            if (ImGui::Button("Stop")) sequencer.stop();
        } else {
            if (ImGui::Button("Play")) sequencer.start();
        }
        ImGui::SameLine();
        ImGui::Text("   BPM: %.0f", sequencer.getBpm());
        ImGui::SameLine();
        if (ImGui::Button("-")) sequencer.setBpm(sequencer.getBpm() - 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("+")) sequencer.setBpm(sequencer.getBpm() + 1.0f);

        ImGui::Separator();

        // === ZONE DE GAUCHE : tracks ===
        // On réserve 250 pixels pour l'inspector à droite.
        const float inspectorWidth = 250.0f;
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const float tracksWidth = availableWidth - inspectorWidth - 8.0f; // 8 = espace entre les deux

        ImGui::BeginChild("TracksZone", ImVec2(tracksWidth, 0), false);

        const int currentStep = sequencer.getCurrentStep();
        const ImVec2 cellSize(28, 28);
        const ImVec4 colorOff        (0.20f, 0.20f, 0.20f, 1.0f);
        const ImVec4 colorOn         (0.20f, 0.75f, 0.75f, 1.0f);
        const ImVec4 colorPlayhead   (0.95f, 0.85f, 0.20f, 1.0f);
        const ImVec4 colorOnPlayhead (0.50f, 0.95f, 0.95f, 1.0f);

        for (int track = 0; track < 4; ++track) {
            ImGui::PushID(track);

            const bool isSelected = (selectedTrack == track);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 1.0f));
            }
            if (ImGui::Button(("Track " + std::to_string(track + 1)).c_str(), ImVec2(70, cellSize.y))) {
                selectedTrack = track;
            }
            if (isSelected) ImGui::PopStyleColor();

            for (int step = 0; step < 16; ++step) {
                ImGui::SameLine();
                ImGui::PushID(step);

                const bool isActive = sequencer.getStep(track, step);
                const bool isPlayhead = sequencer.isPlaying() && (step == currentStep);

                ImVec4 color;
                if (isActive && isPlayhead)      color = colorOnPlayhead;
                else if (isActive)               color = colorOn;
                else if (isPlayhead)             color = colorPlayhead;
                else                             color = colorOff;

                if (ImGui::ColorButton("##cell", color,
                                       ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                                       cellSize))
                {
                    sequencer.setStep(track, step, !isActive);
                }
                ImGui::PopID();
            }

            ImGui::SameLine();
            float vol = players[track].getVolume();
            ImGui::SetNextItemWidth(100);
            if (ImGui::SliderFloat("##vol", &vol, 0.0f, 1.0f, "%.2f")) {
                players[track].setVolume(vol);
            }

            ImGui::SameLine();
            bool muted = players[track].isMuted();
            if (muted) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("M", ImVec2(28, cellSize.y))) players[track].setMuted(false);
                ImGui::PopStyleColor();
            } else {
                if (ImGui::Button("M", ImVec2(28, cellSize.y))) players[track].setMuted(true);
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        // === ZONE DE DROITE : Inspector ===
        ImGui::SameLine();
        ImGui::BeginChild("Inspector", ImVec2(inspectorWidth, 0), true);

        ImGui::Text("Track %d", selectedTrack + 1);
        if (players[selectedTrack].hasFile()) {
            ImGui::Text("%s", players[selectedTrack].getFileName().c_str());
        } else {
            ImGui::Text("(no sample)");
        }

        if (ImGui::Button("Load sample")) {
            // On filtre pour n'afficher que les .wav.
            static const SDL_DialogFileFilter filters[] = {
                { "Fichiers WAV", "wav" }
            };
            // userdata = piste cible, on encode l'int dans un void*.
            SDL_ShowOpenFileDialog(
                fileDialogCallback,
                reinterpret_cast<void*>(static_cast<intptr_t>(selectedTrack)),
                window,           // fenêtre parent (déclarée plus haut dans main)
                filters,
                1,                // nombre de filtres
                nullptr,          // dossier de départ (nullptr = défaut)
                false             // false = sélection unique
            );
        }
        
        static bool delayDummy[4]   = {false, false, false, false};
        static float delayTimeDummy[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        static float delayMixDummy[4]  = {0.0f, 0.0f, 0.0f, 0.0f};

        bool reverseState = players[selectedTrack].isReverse();
        if (ImGui::Checkbox("Reverse", &reverseState)) {
            players[selectedTrack].setReverse(reverseState);
        }
        ImGui::Checkbox("Delay",   &delayDummy[selectedTrack]);
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("Delay time", &delayTimeDummy[selectedTrack], 0.0f, 1.0f, "%.3f");
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("Delay mix",  &delayMixDummy[selectedTrack],  0.0f, 1.0f, "%.3f");

        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // ---- Nettoyage ----
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}