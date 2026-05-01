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

static AudioPlayer players[4];
static Sequencer sequencer;
static constexpr int SAMPLE_RATE = 44100;

static int audioCallback(const void*, void* output,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo*,
                         PaStreamCallbackFlags, void*)
{
    float* out = static_cast<float*>(output);
    const int numFrames = static_cast<int>(framesPerBuffer);

    // 1. Le sequencer fait avancer le temps et déclenche les players.
    sequencer.process(numFrames, SAMPLE_RATE);

    // 2. On mixe les 4 players dans le buffer de sortie.
    //    On commence par mettre tout à zéro.
    for (int i = 0; i < numFrames * 2; ++i) out[i] = 0.0f;

    //    Buffer temporaire pour récupérer la sortie de chaque player.
    static float tempBuffer[256 * 2]; // 256 frames stéréo max
    for (int t = 0; t < 4; ++t) {
        players[t].process(tempBuffer, numFrames);
        for (int i = 0; i < numFrames * 2; ++i) {
            out[i] += tempBuffer[i] * 0.5f; // volume 0.5 par défaut, on changera
        }
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

    SDL_Window* window = SDL_CreateWindow("Drum Machine", 1280, 720, SDL_WINDOW_RESIZABLE);
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
        ImGui::Begin("Test");
        if (sequencer.isPlaying()) {
            if (ImGui::Button("Stop")) sequencer.stop();
        } else {
            if (ImGui::Button("Play")) sequencer.start();
        }
        ImGui::Text("Step courant : %d", sequencer.getCurrentStep());

        float bpm = sequencer.getBpm();
        if (ImGui::SliderFloat("BPM", &bpm, 60.0f, 200.0f)) {
            sequencer.setBpm(bpm);
        }
        ImGui::End();


        ImGui::ShowDemoWindow();

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