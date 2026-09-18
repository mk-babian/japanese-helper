#pragma once
// I don't know where this include came from
// It just kinda appeared here so it will stay
#include <FL/Enumerations.H>

#include <vector>

// Color without a U
// America type shi
struct StyleColors{
    std::string background_string;
    std::string foreground_string;
    std::vector<std::string> colors_vector;

    Fl_Color bg_color; // [special][background]
    Fl_Color fg_color; // [special][foreground]
    
    // Main accent color (vibrant-ish)
    Fl_Color accent_0; // [colors].at(4)
    // Secondary accent color (also, vibrant-ish)
    Fl_Color accent_1; // [colors].at(10)
    // A more vibrant version of accent_0
    Fl_Color accent_2; // [colors].at(6)
    // A more vibrant version of accent_1
    Fl_Color accent_3;
    // A slightly darker color than the background
    Fl_Color bg_accent;
};