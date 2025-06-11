cmake_minimum_required(VERSION 3.21)
project(LostArkPresetConverter LANGUAGES CXX)

set(SLINT_FEATURE_RENDERER_SKIA ON CACHE BOOL "Enable Skia renderer")

find_package(Slint QUIET)
if (NOT Slint_FOUND)
    message("Slint could not be located in the CMake module search path. Downloading it from Git and building it locally")
    include(FetchContent)

    FetchContent_Declare(
            Slint
            GIT_REPOSITORY https://github.com/slint-ui/slint.git
            GIT_TAG release/1
            SOURCE_SUBDIR api/cpp
    )
    FetchContent_MakeAvailable(Slint)
endif (NOT Slint_FOUND)

add_executable(LostArkPresetConverter src/main.cpp src/CusManager.cpp src/DirectoryMonitor.cpp src/Debug.h src/CusManager.h src/OperatingSystemFunctions.h src/DirectoryMonitor.h)
target_link_libraries(LostArkPresetConverter PRIVATE Slint::Slint)
set_target_properties(LostArkPresetConverter PROPERTIES
        WIN32_EXECUTABLE TRUE
        SLINT_EMBED_RESOURCES embed-files
)
slint_target_sources(LostArkPresetConverter ui/app-window.slint)

# On Windows, copy the Slint DLL next to the application binary so that it's found.
if (WIN32)
    add_custom_command(TARGET LostArkPresetConverter POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:LostArkPresetConverter> $<TARGET_FILE_DIR:LostArkPresetConverter> COMMAND_EXPAND_LISTS)
endif ()