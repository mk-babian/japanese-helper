#include <iostream>
#include <fstream>
#include <string>

#include "include/app_state.h"

void load_config(AppState* app){
    // Create class for the config file.
    std::ifstream config("config.ini");
    // Check for errors while opening the file.
    if (!config.is_open()){
        std::cerr << "ERR | Error opening file" << std::endl;
        return; 
    }

    // Line string to store the string.
    std::string line;
    // Loops over all the lines.
    while (std::getline(config, line)){

        /* 
         * Finds the position of the "=".
         * This is useful since we have to separate the config type from the value.
         * (i.e. api_key=916jktg984u62-06o)
        */ 
        size_t eq_pos = line.find("=");

        // Check if find() returned npos, therefore not found.
        if (eq_pos == std::string::npos){
            continue;
        }

        // Separate the key from the value.
        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        // Assign the API key to the app->deepl_key.
        if (key == "deepl_key"){
            app->deepl_key = value;
        }
    }
}

// Handles writing the file.
void save_config(const AppState* app){
    std::ofstream config("config.ini");
    if (!config.is_open()){
        std::cerr << "ERR | Error opening file" << std::endl;
        return;
    }
    
    config << "deepl_key=" + app->deepl_key << std::endl;
}
