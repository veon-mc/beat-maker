#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include "DrumMachine.h"
#include <string>
#include <mutex>
#include <atomic>

// État partagé pour la boîte de dialogue de fichier.
static std::mutex pendingFileMutex;
static std::string pendingFilePath;
static std::atomic<int> pendingFileTrack{-1};

static void SDLCALL fileDialogCallback(void* userdata, const char* const* filelist, int)
{
    if (filelist == nullptr || filelist[0] == nullptr) return;
    const int track = static_cast<int>(reinterpret_cast<intptr_t>(userdata));
    {
        std::lock_guard<std::mutex> lock(pendingFileMutex);
        pendingFilePath = filelist[0];
    }
    pendingFileTrack.store(track);
}

int main(int, char**)
{
    // ---- Init SDL ----
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("SDL_Init: %s\n", SDL_GetError());
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

    // ---- Init du moteur audio ----
    DrumMachine drumMachine;
    if (!drumMachine.init()) return -1;

    // ---- Boucle principale ----
    bool running = true;
    static int selectedTrack = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) running = false;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- Charge un fichier si l'utilisateur en a sélectionné un ---
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
                    drumMachine.loadFile(targetTrack, path);
                }
                pendingFileTrack.store(-1);
            }
        }

        // --- UI plein écran ---
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGuiWindowFlags windowFlags =
              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGui::Begin("Drum Machine", nullptr, windowFlags);

        Sequencer& sequencer = drumMachine.getSequencer();

        // --- Barre du haut ---
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

        // --- Layout : tracks à gauche, inspector à droite ---
        const float inspectorWidth = 250.0f;
        const float tracksWidth = ImGui::GetContentRegionAvail().x - inspectorWidth - 8.0f;
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
            if (isSelected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.85f, 1.0f));
            if (ImGui::Button(("Track " + std::to_string(track + 1)).c_str(), ImVec2(70, cellSize.y))) {
                selectedTrack = track;
            }
            if (isSelected) ImGui::PopStyleColor();

            for (int step = 0; step < 16; ++step) {
                ImGui::SameLine();
                ImGui::PushID(step);
                const bool isActive = sequencer.getStep(track, step);
                const bool isPlayhead = sequencer.isPlaying() && (step == currentStep);
                ImVec4 color = isActive && isPlayhead ? colorOnPlayhead :
                               isActive ? colorOn :
                               isPlayhead ? colorPlayhead : colorOff;
                if (ImGui::ColorButton("##cell", color,
                                       ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                                       cellSize)) {
                    sequencer.setStep(track, step, !isActive);
                }
                ImGui::PopID();
            }

            ImGui::SameLine();
            float vol = drumMachine.getPlayer(track).getVolume();
            ImGui::SetNextItemWidth(100);
            if (ImGui::SliderFloat("##vol", &vol, 0.0f, 1.0f, "%.2f")) {
                drumMachine.getPlayer(track).setVolume(vol);
            }

            ImGui::SameLine();
            bool muted = drumMachine.getPlayer(track).isMuted();
            if (muted) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                if (ImGui::Button("M", ImVec2(28, cellSize.y)))
                    drumMachine.getPlayer(track).setMuted(false);
                ImGui::PopStyleColor();
            } else {
                if (ImGui::Button("M", ImVec2(28, cellSize.y)))
                    drumMachine.getPlayer(track).setMuted(true);
            }

            ImGui::PopID();
        }

        ImGui::EndChild();

        // --- Inspector ---
        ImGui::SameLine();
        ImGui::BeginChild("Inspector", ImVec2(inspectorWidth, 0), true);

        ImGui::Text("Track %d", selectedTrack + 1);
        if (drumMachine.getPlayer(selectedTrack).hasFile()) {
            ImGui::Text("%s", drumMachine.getPlayer(selectedTrack).getFileName().c_str());
        } else {
            ImGui::Text("(no sample)");
        }

        if (ImGui::Button("Load sample")) {
            static const SDL_DialogFileFilter filters[] = { { "Fichiers WAV", "wav" } };
            SDL_ShowOpenFileDialog(
                fileDialogCallback,
                reinterpret_cast<void*>(static_cast<intptr_t>(selectedTrack)),
                window, filters, 1, nullptr, false
            );
        }

        bool reverseState = drumMachine.getPlayer(selectedTrack).isReverse();
        if (ImGui::Checkbox("Reverse", &reverseState)) {
            drumMachine.getPlayer(selectedTrack).setReverse(reverseState);
        }

        bool delayState = drumMachine.getDelay(selectedTrack).isEnabled();
        if (ImGui::Checkbox("Delay", &delayState)) {
            drumMachine.getDelay(selectedTrack).setEnabled(delayState);
        }

        float delayTime = drumMachine.getDelay(selectedTrack).getTime();
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat("Delay time", &delayTime, 0.0f, 1.0f, "%.3f")) {
            drumMachine.getDelay(selectedTrack).setTime(delayTime);
        }

        float delayMix = drumMachine.getDelay(selectedTrack).getMix();
        ImGui::SetNextItemWidth(150);
        if (ImGui::SliderFloat("Delay mix", &delayMix, 0.0f, 1.0f, "%.3f")) {
            drumMachine.getDelay(selectedTrack).setMix(delayMix);
        }

        ImGui::EndChild();
        ImGui::End();

        // --- Rendu ---
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // ---- Nettoyage ----
    drumMachine.shutdown();   // PortAudio
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}