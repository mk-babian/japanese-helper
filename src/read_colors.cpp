#include <fstream>
#include <print>

#include "include/read_colors.h"

// JSON
#include "lib/json.hpp"
using json = nlohmann::json;

StyleColors read_color_from_file(){
    std::string executable_path = get_data_dir("JapaneseHelper").string();
    std::ifstream file(executable_path + "/colors.json");

    StyleColors sc;

    if (!file.is_open()){
        throw std::runtime_error("ERR | Reading from color file failed!\n");
    }

    try {
        json data = json::parse(file);

        if (data.contains("special")){
            sc.background = data["special"]["background"];
            sc.bg_color = hex_to_color(sc.background);
            sc.foreground = data["special"]["foreground"];
            sc.fg_color = hex_to_color(sc.foreground);

            // std::println("INFO | Read colors from file: bg = {}, fg = {}", sc.background, sc.foreground);
        }

        if (data.contains("colors")){
            for (json::iterator it = data["colors"].begin(); it != data["colors"].end(); ++it){
                std::string key = it.key();
                sc.colors.push_back(it.value());
                // std::println("INFO | Read colors from file: {}", sc.colors.back());
            }
        }
    } catch (const json::parse_error& e){
        throw std::runtime_error(std::format("ERR | Error parsing colors JSON file: {}\n", e.what()));
    }

    sc.accent_0 = hex_to_color(sc.colors.at(4));
    sc.accent_1 = hex_to_color(sc.colors.at(10));
    sc.accent_2 = hex_to_color(sc.colors.at(1));

    return sc;
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