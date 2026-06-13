#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Multiline_Output.H>

#include <vector>
#include <string>

#include "speech_to_text.h"
#include "history_circ_buffer.h"

struct AppState{
    // Everything related to the settings window
    Fl_Window* settings_win;
    Fl_Input* settings_key_input = nullptr;
    Fl_Input* settings_email_input = nullptr;
    int selected_settings_win = 0;
    Fl_Group* settings_content;

    Fl_Window* info_win;
    Fl_Group* info_content;
    int selected_info_win = 0;
    Fl_Multiline_Output* info_text = nullptr;

    // The buttons on the left of the info window
    Fl_Button* general_info_btn;
    Fl_Button* api_info_btn;
    Fl_Button* whisper_info_btn;

    // The buttons on the left of the settings window
    Fl_Button* general_settings_btn;
    Fl_Button* history_settings_btn;
    Fl_Button* api_settings_btn;
    Fl_Button* stt_settings_btn;

    // Whisper.cpp stuff
    Fl_Choice* whisper_model_selector;
    Fl_Button* install_whisper_model;
    int selected_model = -1;

    // Input device selection for speech-to-text
    Fl_Choice* whisper_device_selector = nullptr;
    // PortAudio device index of the selected input device. paNoDevice means "use the default".
    // Resolved from selected_input_device_name at runtime; indices are not stable across runs.
    PaDeviceIndex selected_input_device = paNoDevice;
    // The persisted identity of the chosen device. Empty means "use the default".
    std::string selected_input_device_name;

    // Everything related to the history window
    Fl_Window* history_win;
    Fl_Scroll* history_scroll;
    CircularBuffer* history_buf;

    // Main window widgets
    Fl_Window* main_win;
    Fl_Button* stt_btn;
    Fl_Button* search_btn;
    Fl_Choice* api_selector;

    // Main window input and output
    Fl_Input* input;
    Fl_Multiline_Output* output;

    // The PortAudio stream data for the speech-to-text functionality
    StreamData stream_data;

    // The currently selected API (0 for jisho, 1 for deepl, and so on...)
    int selected_api = 0;
    bool key_shown = false;
    std::string deepl_key;
    // Optional email sent to MyMemory to raise the daily usage limit
    std::string mymemory_email;
};