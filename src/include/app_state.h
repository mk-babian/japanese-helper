#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Output.H>

struct AppState{
    Fl_Input* input;
    Fl_Multiline_Output* output;
    int selected_api = 0;
};
