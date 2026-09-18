#pragma once
// I don't know where this include came from
// It just kinda appeared here so it will stay
#include <FL/Enumerations.H>

#include <vector>

// Color without a U
// America type shi
struct StyleColors{
    std::string background;
    std::string foreground;
    std::vector<std::string> colors;

    Fl_Color bg_color;
    Fl_Color fg_color;
    Fl_Color accent_0;
    Fl_Color accent_1;
    Fl_Color accent_2;
    Fl_Color accent_3;
};