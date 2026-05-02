#include "MainWindow.h"
#include "imgui.h"
#include <string>

MainWindow::MainWindow(DrumMachine& dm, SDL_Window* window)
    : drumMachine(dm), sdlWindow(window) {}

void MainWindow::draw()
{
    consumePendingFileLoad();

    // Fenêtre ImGui occupant toute la viewport, sans barre de titre.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGuiWindowFlags flags =
          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("Drum Machine", nullptr, flags);

    drawTopBar();
    ImGui::Separator();

    const float inspectorWidth = 250.0f;
    const float tracksWidth = ImGui::GetContentRegionAvail().x - inspectorWidth - 8.0f;

    ImGui::BeginChild("TracksZone", ImVec2(tracksWidth, 0), false);
    drawTracks();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("Inspector", ImVec2(inspectorWidth, 0), true);
    drawInspector();
    ImGui::EndChild();

    ImGui::End();
}

void MainWindow::drawTopBar()
{
    Sequencer& sequencer = drumMachine.getSequencer();

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
}

void MainWindow::drawTracks()
{
    Sequencer& sequencer = drumMachine.getSequencer();
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
            ImVec4 color = (isActive && isPlayhead) ? colorOnPlayhead :
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
}

void MainWindow::drawInspector()
{
    AudioPlayer& player = drumMachine.getPlayer(selectedTrack);
    Delay& delay = drumMachine.getDelay(selectedTrack);

    ImGui::Text("Track %d", selectedTrack + 1);
    if (player.hasFile()) {
        ImGui::Text("%s", player.getFileName().c_str());
    } else {
        ImGui::Text("(no sample)");
    }

    if (ImGui::Button("Load sample")) {
        openFileDialog(selectedTrack);
    }

    bool reverseState = player.isReverse();
    if (ImGui::Checkbox("Reverse", &reverseState)) {
        player.setReverse(reverseState);
    }

    bool delayState = delay.isEnabled();
    if (ImGui::Checkbox("Delay", &delayState)) {
        delay.setEnabled(delayState);
    }

    float delayTime = delay.getTime();
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderFloat("Delay time", &delayTime, 0.0f, 1.0f, "%.3f")) {
        delay.setTime(delayTime);
    }

    float delayMix = delay.getMix();
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderFloat("Delay mix", &delayMix, 0.0f, 1.0f, "%.3f")) {
        delay.setMix(delayMix);
    }
}

void MainWindow::openFileDialog(int track)
{
    static const SDL_DialogFileFilter filters[] = { { "Fichiers WAV", "wav" } };

    // Astuce userdata : on encode (this, track) en stockant juste la piste cible
    // dans pendingFileTrack en avance. La callback retrouve l'instance via 'this'.
    pendingFileTrack.store(-2); // -2 = "en attente, pas encore choisi"
    SDL_ShowOpenFileDialog(
        fileDialogCallback,
        this,                    // userdata = pointeur vers MainWindow
        sdlWindow,
        filters, 1, nullptr, false
    );
    // On note la piste cible localement via une variable membre annexe :
    pendingFileTargetTrack = track;
}

void SDLCALL MainWindow::fileDialogCallback(void* userdata,
                                            const char* const* filelist,
                                            int)
{
    auto* self = static_cast<MainWindow*>(userdata);
    if (filelist == nullptr || filelist[0] == nullptr) {
        self->pendingFileTrack.store(-1);
        return;
    }
    {
        std::lock_guard<std::mutex> lock(self->pendingFileMutex);
        self->pendingFilePath = filelist[0];
    }
    self->pendingFileTrack.store(self->pendingFileTargetTrack);
}

void MainWindow::consumePendingFileLoad()
{
    const int target = pendingFileTrack.load();
    if (target < 0) return;

    std::string path;
    {
        std::lock_guard<std::mutex> lock(pendingFileMutex);
        path = pendingFilePath;
        pendingFilePath.clear();
    }
    if (!path.empty()) {
        drumMachine.loadFile(target, path);
    }
    pendingFileTrack.store(-1);
}