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

static AudioPlayer players[4];
static Sequencer sequencer;
static constexpr int SAMPLE_RATE = 44100;
static Mixer mixer;

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
    // Charge un fichier sur la piste 0 (kick) pour tester.
    players[0].loadFile("Kick1.wav", SAMPLE_RATE);

    // Câble le player 0 au sequencer.
    for (int t = 0; t < 4; ++t) sequencer.setPlayer(t, &players[t]);

    // Active quelques pas sur la piste 0 pour entendre quelque chose au lancement.
    sequencer.setStep(0, 0, true);
    sequencer.setStep(0, 4, true);
    sequencer.setStep(0, 8, true);
    sequencer.setStep(0, 12, true);

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
        ImGui::Text("(no sample)");

        if (ImGui::Button("Load sample")) {
            // À câbler à l'étape suivante.
        }

        static bool reverseDummy[4] = {false, false, false, false};
        static bool delayDummy[4]   = {false, false, false, false};
        static float delayTimeDummy[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        static float delayMixDummy[4]  = {0.0f, 0.0f, 0.0f, 0.0f};

        ImGui::Checkbox("Reverse", &reverseDummy[selectedTrack]);
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