cmake_minimum_required(VERSION 3.21...4.3.1)
project(translator C CXX)

set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Wall and Wextra (for debugging)
add_compile_options(-Wall -Wextra)

# Used to download dependencies
include(FetchContent)

set(CURL_INCLUDE_DIR "C:/libs/curl/include")
set(CURL_LIBRARIES "C:/libs/curl/lib/libcurl.lib")

# --- CURL ---
find_package(CURL QUIET)
if(NOT CURL_FOUND)
    message(STATUS "CURL not found, trying to fetch...")
    FetchContent_Declare(
        curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_7_1 # Or your preferred version
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/curl-src
    )
    set(BUILD_CURL_EXE OFF CACHE INTERNAL "")
    set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    set(CURL_USE_LIBPSL OFF CACHE INTERNAL "")
    
    if(WIN32)
        set(CURL_USE_SCHANNEL ON CACHE INTERNAL "")
    else()
        set(CURL_USE_OPENSSL ON CACHE INTERNAL "")
    endif()

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
    # FetchContent doesn't set find_package variables, so set them manually.
    # fltk_BINARY_DIR is needed because FL/fl_config.h is generated there.
    set(FLTK_INCLUDE_DIR ${fltk_SOURCE_DIR} ${fltk_BINARY_DIR})
    set(FLTK_LIBRARIES fltk fltk_images)
endif()

# --- PortAudio ---
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)
endif()

if(NOT PORTAUDIO_FOUND)
    message(STATUS "PortAudio not found, trying to fetch...")

    set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

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
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp)

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
endif()

# Files for executable
add_executable(translator
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/api.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/callbacks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/speech_to_text.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/get_exec_path.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/history_circ_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/truncate_label.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/get_data_dir.cpp
    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/resources.rc>
)

if(WIN32)
    target_compile_definitions(translator PRIVATE WIN32 _WIN32)
endif()

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
    add_custom_command(TARGET translator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:translator>
            $<TARGET_FILE_DIR:translator>
        COMMAND_EXPAND_LISTS
        COMMENT "Copying required runtime DLLs to the build directory..."
    )
    
    add_custom_target(download_whisper_model
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/models/download-ggml-model.cmd base
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/whisper.cpp/models
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/ggml-base.bin
        ${CMAKE_CURRENT_BINARY_DIR}/whisper.cpp/models/ggml-base.bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp
    COMMENT "Downloading Whisper base model..."
    )
else()
    add_custom_target(download_whisper_model
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/models/download-ggml-model.cmd base
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/whisper.cpp/models
    COMMAND ${CMAKE_COMMAND} -E rename
        ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp/ggml-base.bin          # <-- no /models/
        ${CMAKE_CURRENT_BINARY_DIR}/whisper.cpp/models/ggml-base.bin
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/whisper.cpp
    COMMENT "Downloading Whisper base model..."
    )
endif()

add_custom_command(TARGET translator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/images
        ${CMAKE_BINARY_DIR}/images
    COMMENT "Copying images directory to build..."
)