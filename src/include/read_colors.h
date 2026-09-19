#pragma once

#include "app_state.h"
#include "get_data_dir.h"

StyleColors read_color_from_file();
Fl_Color hex_to_color(const std::string& hex);
Fl_Color readable_label_color(Fl_Color bg);
void set_widget_fill(Fl_Widget* w, Fl_Color fill);
void set_theme(StyleColors& sc);