#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include "DrumMachine.h"
#include "MainWindow.h"
#include <cstdio>

/**
 * @brief Application entry point.
 *
 * Sets up SDL window, ImGui context, the DrumMachine audio engine
 * and the MainWindow UI, then runs the rendering loop. Cleans up
 * everything on exit.
 *
 * @return 0 on normal exit, -1 if SDL or audio initialization failed.
 */
int main(int, char**)
{
    // ---- SDL setup ----
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Drum Machine", 1280, 250, SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, 1100, 220);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

    // ---- ImGui setup ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // ---- Audio engine setup ----
    DrumMachine drumMachine;
    if (!drumMachine.init()) return -1;

    // ---- UI setup ----
    MainWindow mainWindow(drumMachine, window);

    // ---- Main loop ----
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        mainWindow.draw();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // ---- Cleanup ----
    drumMachine.shutdown();
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}