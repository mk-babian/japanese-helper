#include <print>
#include <fstream>
#include <string>
#include <filesystem>

#include "include/app_state.h"
#include "include/get_exec_path.h"

void load_config(AppState* app){
    std::string executable_path = get_executable_path().parent_path().string();
    std::println("INFO | Executable directory: {}", executable_path);
    // Create class for the config file.
    std::ifstream config(executable_path + "/config.ini");
    // Check for errors while opening the file.
    if (!config.is_open()){
        std::println("ERR | Error while LOADING config file!");
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
    std::string executable_path = get_executable_path().parent_path().string();
    std::ofstream config(executable_path + "/config.ini");
    if (!config.is_open()){
        std::println("ERR | Error while SAVING config file!");
        return;
    }
    
    config << "deepl_key=" + app->deepl_key << std::endl;
}
