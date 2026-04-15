cmake_minimum_required(VERSION 4.3.1)
project(translator CXX)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Wall and Wextra (for debugging)
add_compile_options(-Wall -Wextra)

# Used to download dependencies
include(FetchContent)

# --- CURL ---
find_package(CURL QUIET)
if(NOT CURL_FOUND)
    message(STATUS "CURL not found, trying to fetch...")
    FetchContent_Declare(
        curl
        URL https://github.com/curl/curl/releases/download/curl-8_19_0/curl-8.19.0.tar.gz 
    )
    FetchContent_MakeAvailable(curl)
endif()

# --- FLTK ---
find_package(FLTK QUIET)
if(NOT FLTK_FOUND)
    message(STATUS "FLTK not found, trying to fetch...")
    FetchContent_Declare(
        fltk
        GIT_REPOSITORY https://github.com/fltk/fltk.git
        GIT_TAG release-1.4.4
    )
    FetchContent_MakeAvailable(fltk)
endif()

# --- PortAudio ---
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
if(NOT PORTAUDIO_FOUND)
    message(STATUS "PortAudio not found, trying to fetch...")
    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG v19.7.0
    )
    FetchContent_MakeAvailable(portaudio)
    set(PORTAUDIO_LIBRARIES portaudio)
    set(PORTAUDIO_INCLUDE_DIRS ${portaudio_SOURCE_DIR}/include)
endif()

# Whisper.cpp
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Disable whisper tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Disable whisper examples" FORCE)
add_subdirectory(whisper.cpp)

# Windows-only icon stuff using resources.rc (found in `src`)
if(WIN32)
    set(RESOURCE_RC  ${CMAKE_CURRENT_SOURCE_DIR}/resources.rc)
    set(RESOURCE_OBJ ${CMAKE_CURRENT_BINARY_DIR}/resources.o)

    add_custom_command(
        OUTPUT  ${RESOURCE_OBJ}
        COMMAND windres ${RESOURCE_RC} -o ${RESOURCE_OBJ}
        DEPENDS ${RESOURCE_RC} ${CMAKE_CURRENT_SOURCE_DIR}/../app_icon.ico
        COMMENT "Compiling Windows resources..."
    )

    target_sources(translator PRIVATE ${RESOURCE_OBJ})
endif()

# Files for executable
add_executable(translator
    main.cpp
    api.cpp
    settings.cpp
    callbacks.cpp
    speech_to_text.cpp
)

# Combine includes
target_include_directories(translator PRIVATE
    ${FLTK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/ggml/include
    ${PORTAUDIO_INCLUDE_DIRS}
)

# Combine libs
target_link_libraries(translator PRIVATE
    ${FLTK_LIBRARIES}
    CURL::libcurl
    ${PORTAUDIO_LIBRARIES}
    whisper
    ggml
    ggml-base
    ggml-cpu
    stdc++exp
    m
)

if(WIN32)
    add_custom_target(download_whisper_model
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/models/download-ggml-model.cmd base
        COMMAND ${CMAKE_COMMAND} -E rename
            ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/models/ggml-base.bin
            ${CMAKE_CURRENT_BINARY_DIR}/whisper.cpp/models/ggml-base.bin
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp
        COMMENT "Downloading Whisper base model..."
    )
else()
    add_custom_target(download_whisper_model
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/models/download-ggml-model.sh base
        COMMAND ${CMAKE_COMMAND} -E rename
            ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/models/ggml-base.bin
            ${CMAKE_CURRENT_BINARY_DIR}/whisper.cpp/models/ggml-base.bin
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp
        COMMENT "Downloading Whisper base model..."
    )
endif()

add_custom_command(TARGET translator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/../images
        ${CMAKE_BINARY_DIR}/images
    COMMENT "Copying images directory to build..."
)
