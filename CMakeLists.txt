cmake_minimum_required(VERSION 3.21...4.3.1)
project(translator C CXX)

option(TRANSLATOR_STATIC  "Link the runtime and standard libraries statically" OFF)
option(TRANSLATOR_NO_CONSOLE "Hide the console window (Windows WIN32 subsystem)" OFF)

set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Enable Wall and Wextra (for debugging)
add_compile_options(-Wall -Wextra)

if(TRANSLATOR_STATIC)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(WIN32)
        add_link_options(-static -static-libgcc -static-libstdc++)
    else()
        add_link_options(-static-libgcc -static-libstdc++)
    endif()
endif()

# --- Dependencies (must be installed on the system) ---
find_package(PkgConfig REQUIRED)
find_package(CURL REQUIRED)
find_package(FLTK REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)
# System FLTK may be built with cairo support, so its public header needs
# cairo.h even though we don't use cairo directly.
pkg_check_modules(CAIRO QUIET cairo)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/download_whisper_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/global_hotkey.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/read_colors.cpp
    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/ocr.cpp>
    $<$<BOOL:${WIN32}>:${CMAKE_CURRENT_SOURCE_DIR}/resources.rc>
)

if(WIN32)
    target_compile_definitions(translator PRIVATE WIN32 _WIN32)
endif()

if(TRANSLATOR_NO_CONSOLE)
    if(WIN32)
        set_target_properties(translator PROPERTIES WIN32_EXECUTABLE TRUE)
    elseif(APPLE)
        set_target_properties(translator PROPERTIES MACOSX_BUNDLE TRUE)
    endif()
endif()

# Combine includes
target_include_directories(translator PRIVATE
    ${FLTK_INCLUDE_DIR}
    ${CAIRO_INCLUDE_DIRS}
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
    target_link_libraries(translator PRIVATE gdiplus gdi32 user32 winmm setupapi)
endif()

add_custom_command(TARGET translator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/images
        ${CMAKE_BINARY_DIR}/images
    COMMENT "Copying images directory to build..."
)

add_custom_command(TARGET translator POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/whisper.cpp/models
        ${CMAKE_BINARY_DIR}/whisper.cpp/models
    COMMENT "Copying whisper.cpp models directory to build..."
)

# --- Install / CPack ---
install(TARGETS translator RUNTIME DESTINATION .)

if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:translator> DESTINATION .)
endif()

install(DIRECTORY ${CMAKE_SOURCE_DIR}/images/
    DESTINATION images
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/whisper.cpp/models/
    DESTINATION whisper.cpp/models
)

if(WIN32)
    set(CPACK_GENERATOR "ZIP")
else()
    set(CPACK_GENERATOR "TGZ")
endif()

set(CPACK_PACKAGE_NAME "translator")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
include(CPack)