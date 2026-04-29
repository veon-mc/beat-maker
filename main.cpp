#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <portaudio.h>
#include <cstdio>
#include <cmath>

static int audioCallback(const void* /*input*/,
                         void* output,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* /*timeInfo*/,
                         PaStreamCallbackFlags /*statusFlags*/,
                         void* /*userData*/)
{
    float* out = static_cast<float*>(output);

    // Variables persistantes entre deux appels de la callback :
    // 'static' fait qu'elles gardent leur valeur entre les appels.
    static double phase = 0.0;
    const double sampleRate = 44100.0;
    const double frequency = 440.0;       // La 440 Hz
    const double phaseIncrement = 2.0 * M_PI * frequency / sampleRate;
    const float amplitude = 0.2f;          // ~20% du max, pour ne pas exploser les oreilles

    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float sample = amplitude * static_cast<float>(std::sin(phase));
        *out++ = sample; // canal gauche
        *out++ = sample; // canal droit
        phase += phaseIncrement;
        if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI; // évite que phase grandisse à l'infini
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