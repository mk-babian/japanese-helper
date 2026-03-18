#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Multiline_Output.H>
#include <string>

struct AppState{
    Fl_Window* settings_win;
    Fl_Button* vtt_btn;
    Fl_Button* search_btn;
    Fl_Choice* api_selector;
    Fl_Input* settings_key_input;
    Fl_Input* input;
    Fl_Multiline_Output* output;

    int selected_api = 0;
    std::string deepl_key;
};
