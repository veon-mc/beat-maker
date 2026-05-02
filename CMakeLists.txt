cmake_minimum_required(VERSION 3.20)
project(beatbox_64889)

set(CMAKE_CXX_STANDARD 20)

include_directories(
        "libraries/imgui/backends"
        "libraries/imgui"
        "libraries/portaudio/include"
        "libraries/sdl/include"
        "src"
        "src/audio"
        "src/ui"
)

add_executable(drum_machine
        main.cpp
        src/audio/AudioPlayer.cpp
        src/audio/Delay.cpp
        src/audio/Mixer.cpp
        src/audio/Sequencer.cpp
        src/audio/DrumMachine.cpp
        src/ui/MainWindow.cpp
        libraries/imgui/imgui.cpp
        libraries/imgui/imgui_demo.cpp
        libraries/imgui/imgui_draw.cpp
        libraries/imgui/imgui_widgets.cpp
        libraries/imgui/imgui_tables.cpp
        libraries/imgui/backends/imgui_impl_sdl3.cpp
        libraries/imgui/backends/imgui_impl_sdlrenderer3.cpp
        src/audio/Mixer.cpp
        src/audio/Mixer.h
        src/audio/Mixer.h
        src/audio/AudioPlayer.cpp
        src/audio/Delay.cpp
        src/audio/Sequencer.cpp
        src/audio/DrumMachine.cpp
        src/ui/MainWindow.cpp
        src/audio/AudioFileReader.h
        src/audio/AudioFileReader.cpp
        src/audio/AudioPlayer.h
        src/audio/Sequencer.h
        src/audio/Delay.h
        src/audio/Preferences.h
        src/audio/Preferences.cpp
        src/audio/DrumMachine.h
)

if (APPLE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_SOURCE_DIR}/../libraries/sdl/lib/macos/SDL3.framework")
    target_link_libraries(drum_machine PRIVATE
            "${CMAKE_SOURCE_DIR}/libraries/sdl/lib/macos/SDL3.framework"
            "-framework AudioToolbox"
            "-framework CoreAudio"
            "-framework Carbon"
            "${CMAKE_SOURCE_DIR}/libraries/portaudio/lib/macos/libportaudio.a")
elseif (WIN32)
    target_link_libraries(drum_machine PRIVATE
            "${CMAKE_SOURCE_DIR}/libraries/sdl/lib/win32/sdl3.dll"
            "${CMAKE_SOURCE_DIR}/libraries/portaudio/lib/win32/portaudio_x64.dll"
    )

    add_custom_command(TARGET drum_machine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/libraries/sdl/lib/win32/sdl3.dll"
            "$<TARGET_FILE_DIR:drum_machine>"
    )
    add_custom_command(TARGET drum_machine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/libraries/portaudio/lib/win32/portaudio_x64.dll"
            "$<TARGET_FILE_DIR:drum_machine>"
    )
elseif (UNIX)
    target_link_libraries(drum_machine PRIVATE
            "-lGL"
            "-ldl"
            "-ljack"
            "${CMAKE_SOURCE_DIR}/libraries/sdl/lib/linux-x86_64/libSDL3.a"
            "${CMAKE_SOURCE_DIR}/libraries/portaudio/lib/linux-x86_64/libportaudio.a")
endif ()
