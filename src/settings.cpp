#include <iostream>
#include <fstream>
#include <string>

#include "include/app_state.h"

void load_config(AppState* app){
    // write into the struct
    std::ifstream config("config.ini");
    if (!config.is_open()){
        std::cerr << "ERR | Error opening file" << std::endl;
        return; 
    }

    std::string line;
    while (std::getline(config, line)){
        size_t eq_pos = line.find("=");

        if (eq_pos == std::string::npos){
            continue;
        }

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        if (key == "deepl_key"){
            app->deepl_key = value;
        }
    }
}

void save_config(const AppState* app){
    std::ofstream config("config.ini");
    if (!config.is_open()){
        std::cerr << "ERR | Error opening file" << std::endl;
        return;
    }
    
    config << "deepl_key=" + app->deepl_key << std::endl;
}
