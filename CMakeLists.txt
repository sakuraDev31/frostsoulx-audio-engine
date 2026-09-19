cmake_minimum_required(VERSION 3.20)
project(frostsoulx_immersive_audio VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FROSTSOULX_USE_STEAM_AUDIO
    "Enable the vendored Steam Audio C API backend"
    OFF)
option(FROSTSOULX_BUILD_TESTS
    "Build host-side engine tests"
    ON)

add_library(frostsoulx_immersive_audio STATIC
    src/immersive_audio_engine.cpp
    src/dsp/partitioned_convolver.cpp
    src/spatial/ambisonics.cpp
    src/spatial/hrtf.cpp
    src/spatial/spatial_renderer.cpp
)
target_include_directories(frostsoulx_immersive_audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(frostsoulx_immersive_audio PUBLIC cxx_std_17)
target_compile_options(frostsoulx_immersive_audio PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(FROSTSOULX_USE_STEAM_AUDIO)
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        set(FROSTSOULX_STEAMAUDIO_LIB_DIR "android-armv8")
    elseif(ANDROID_ABI STREQUAL "x86_64")
        set(FROSTSOULX_STEAMAUDIO_LIB_DIR "android-x64")
    else()
        message(FATAL_ERROR "Steam Audio 4.8.1 is available for Android arm64-v8a and x86_64 only")
    endif()
    add_library(phonon SHARED IMPORTED)
    set_target_properties(phonon PROPERTIES
        IMPORTED_LOCATION
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamaudio_sdk/lib/${FROSTSOULX_STEAMAUDIO_LIB_DIR}/libphonon.so
        INTERFACE_INCLUDE_DIRECTORIES
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamaudio_sdk/include)
    target_compile_definitions(frostsoulx_immersive_audio PRIVATE
        FROSTSOULX_STEAM_AUDIO_AVAILABLE=1)
    target_link_libraries(frostsoulx_immersive_audio PRIVATE phonon)
endif()

if(FROSTSOULX_BUILD_TESTS)
    enable_testing()
    add_executable(frostsoulx_immersive_audio_tests
        tests/test_immersive_audio_engine.cpp)
    target_link_libraries(frostsoulx_immersive_audio_tests PRIVATE
        frostsoulx_immersive_audio)
    add_test(NAME frostsoulx_immersive_audio_tests
        COMMAND frostsoulx_immersive_audio_tests)
endif()
