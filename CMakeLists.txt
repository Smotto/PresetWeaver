cmake_minimum_required(VERSION 3.21)
project(PresetWeaver LANGUAGES C CXX)

# Using either Skia or qt
set(SLINT_FEATURE_RENDERER_SKIA ON CACHE BOOL "Enable Skia renderer")
set(ENV{SLINT_BACKEND} winit-skia)

#set(ENV{SLINT_BACKEND} qt)
#set(Qt6_DIR "C:/SoftwareDevelopmentKits/Qt/6.9.0/mingw_64/lib/cmake/Qt6")
#set(CMAKE_PREFIX_PATH "${Qt6_DIR}" ${CMAKE_PREFIX_PATH})

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

add_executable(PresetWeaver src/main.cpp src/CusManager.cpp src/DirectoryMonitor.cpp src/FileInfo.cpp src/xxhash.c
                                      src/Debug.h src/CusManager.h src/OperatingSystemFunctions.h src/DirectoryMonitor.h src/FileInfo.h src/xxhash.h)
target_link_libraries(PresetWeaver PRIVATE Slint::Slint)
set_target_properties(PresetWeaver PROPERTIES
        WIN32_EXECUTABLE TRUE
        SLINT_EMBED_RESOURCES embed-files
)
slint_target_sources(PresetWeaver ui/app-window.slint)

# On Windows, copy the Slint DLL next to the application binary so that it's found.
if (WIN32)
    add_custom_command(TARGET PresetWeaver POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:PresetWeaver> $<TARGET_FILE_DIR:PresetWeaver> COMMAND_EXPAND_LISTS)
endif ()