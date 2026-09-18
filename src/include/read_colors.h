#pragma once

#include "app_state.h"
#include "get_data_dir.h"

StyleColors read_color_from_file();
Fl_Color hex_to_color(const std::string& hex);