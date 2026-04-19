cmake_minimum_required(VERSION 3.21...4.3.1)
project(translator C CXX)

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
    # Remove unnecessary parts of curl
    set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBSSH2 OFF CACHE BOOL "" FORCE)
    set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
    set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
    set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
    set(USE_NGHTTP2 OFF CACHE BOOL "" FORCE)
    set(CURL_USE_LIBIDN2 OFF CACHE BOOL "" FORCE)
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

    # Create a patch script to bump the minimum CMake version from 3.0 to 3.5
    file(WRITE ${CMAKE_BINARY_DIR}/patch_portaudio.cmake "
        file(READ CMakeLists.txt content)
        string(REPLACE \"VERSION 3.0\" \"VERSION 3.5\" content \"\${content}\")
        file(WRITE CMakeLists.txt \"\${content}\")
        ")

    FetchContent_Declare(
        portaudio
        GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
        GIT_TAG v19.7.0
        # Apply the patch immediately after downloading
        PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/patch_portaudio.cmake
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
endif()

# Files for executable
add_executable(translator
    main.cpp
    api.cpp
    settings.cpp
    callbacks.cpp
    speech_to_text.cpp
    get_exec_path.cpp
    ${RESOURCE_OBJ}
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
