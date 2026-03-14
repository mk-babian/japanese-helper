#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Multiline_Output.H>
#include <string>

struct AppState{
    Fl_Input* input;
    Fl_Multiline_Output* output;

    int selected_api = 0;

    std::string deepl_key;

    Fl_Window* settings_win;
    Fl_Input* settings_key_input;
};
