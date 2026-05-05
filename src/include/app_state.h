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
    Fl_Window* settings_win;
    Fl_Input* settings_key_input;
    int selected_settings_win = 0;
    Fl_Group* settings_content;

    Fl_Button* general_settings_btn;
    Fl_Button* history_settings_btn;
    Fl_Button* api_settings_btn;
    Fl_Button* stt_settings_btn;

    Fl_Choice* whisper_model_selector;
    Fl_Button* install_whisper_model;

    Fl_Window* history_win;
    Fl_Scroll* history_scroll;
    CircularBuffer* history_buf;

    Fl_Button* stt_btn;
    Fl_Button* search_btn;
    Fl_Choice* api_selector;

    Fl_Input* input;
    Fl_Multiline_Output* output;

    StreamData stream_data;

    int selected_api = 0;
    std::string deepl_key;
};