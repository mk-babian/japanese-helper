#include <fstream>
#include <print>

// JSON
#include "lib/json.hpp"
using json = nlohmann::json;

#include "include/app_state.h"

StyleColors read_color_from_file(std::ifstream file){
    StyleColors sc;
    if (!file.is_open()) std::println("W | Couldn't read from color file."); return sc;

    try {
        json data = json::parse(file);

        if (data.contains("special")){
            sc.background = data["special"]["background"];
            sc.foreground = data["special"]["foreground"];

            std::println("I | Read colors from file: bg = {}, fg = {}", sc.background, sc.foreground);
        }

        if (data.contains("colors")){
            for (json::iterator it = data["colors"].begin(); it != data["colors"].end(); ++it){
                std::string key = it.key();
                sc.colors.push_back(it.value());
            }
        }
    } catch (const json::parse_error& e){
        std::println("W | Parse error: {}", e.what());
        return sc;
    }

    return sc;
}