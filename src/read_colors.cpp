#include <filesystem>
#include <fstream>
#include <print>

#include "include/read_colors.h"

// JSON
#include "lib/json.hpp"
using json = nlohmann::json;

void write_default_colors() {
    std::string executable_path = get_data_dir("JapaneseHelper").string();

    json default_colors = {
        {"special", {
            {"background", "#fcfcfc"},
            {"foreground", "#3f3f3f"},
            {"cursor", "#3f3f3f"}
        }},
        {"colors", {
            {"color0", "#fcfcfc"},
            {"color1", "#c3c5c3"},
            {"color2", "#6391aa"},
            {"color3", "#7e94ab"},
            {"color4", "#94a0ae"},
            {"color5", "#a9b2bd"},
            {"color6", "#1641aa"},
            {"color7", "#3f3f3f"},
            {"color8", "#bdbdbd"},
            {"color9", "#c3c5c3"},
            {"color10", "#6391aa"},
            {"color11", "#7e94ab"},
            {"color12", "#94a0ae"},
            {"color13", "#a9b2bd"},
            {"color14", "#1641aa"},
            {"color15", "#3f3f3f"}
        }}
    };

    std::ofstream file(executable_path + "/colors.json");
    if (!file.is_open()){
        throw std::runtime_error("ERR | Writing to color file failed!\n");
    }

    file << default_colors.dump(4) << std::endl;
}

StyleColors read_color_from_file(){
    std::string data_path = get_data_dir("JapaneseHelper").string();

    StyleColors sc;

    // If the color file doesn't exist yet, write the defaults first so the
    // app can start with a sane theme instead of crashing.
    if (!std::filesystem::exists(data_path + "/colors.json")){
        write_default_colors();
    }

    std::ifstream file(data_path + "/colors.json");

    if (!file.is_open()){
        throw std::runtime_error("ERR | Reading from color file failed!\n");
    }

    try {
        json data = json::parse(file);

        if (data.contains("special")){
            sc.background_string = data["special"]["background"];
            sc.bg_color = hex_to_color(sc.background_string);
            sc.foreground_string = data["special"]["foreground"];
            sc.fg_color = hex_to_color(sc.foreground_string);
        }

        if (data.contains("colors")){
            const std::size_t n = data["colors"].size();
            for (std::size_t i = 0; i < n; ++i){
                sc.colors_vector.push_back(data["colors"]["color" + std::to_string(i)]);
            }
        }
    } catch (const json::parse_error& e){
        throw std::runtime_error(std::format("ERR | Error parsing colors JSON file: {}\n", e.what()));
    }

    sc.accent_0 = hex_to_color(sc.colors_vector.at(4));
    sc.accent_1 = hex_to_color(sc.colors_vector.at(10));
    sc.accent_2 = hex_to_color(sc.colors_vector.at(6));
    sc.accent_3 = hex_to_color(sc.colors_vector.at(11));
    sc.bg_accent = hex_to_color(sc.colors_vector.at(8));

    set_theme(sc);

    return sc;
}

void set_theme(StyleColors& sc) {
    uchar r, g, b;
    Fl::get_color(sc.bg_color, r, g, b);
    double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b; // 0–255 scale, fast approx
    sc.light_theme = luminance > 140;
}

Fl_Color hex_to_color(const std::string& hex) {
    std::string h = hex;
    // Remove the #
    if (h[0] == '#') {
        h = h.substr(1);
    }
    unsigned long rgb = std::stoul(h, nullptr, 16);
    return (Fl_Color)(rgb << 8);
}

Fl_Color readable_label_color(Fl_Color bg) {
    uchar r, g, b;
    Fl::get_color(bg, r, g, b);
    double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b; // 0–255 scale, fast approx
    return luminance > 140 ? FL_BLACK : FL_WHITE;
}

void set_widget_fill(Fl_Widget* w, Fl_Color fill) {
    w->color(fill);
    w->labelcolor(readable_label_color(fill));
    w->selection_color(readable_label_color(fill));
    w->redraw();
}