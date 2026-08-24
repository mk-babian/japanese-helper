#include <print>
#include <fstream>
#include <string>
#include <filesystem>

#include "include/app_state.h"
#include "include/get_data_dir.h"
// i don't use this anymore ↓
#include "include/get_exec_path.h"
#include "include/history_circ_buffer.h"
#include "include/ocr.h"

void load_config(AppState* app){
    std::string executable_path = get_data_dir("JapaneseHelper").string();
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

        if (key == "mymemory_email"){
            app->mymemory_email = value;
        }

        if (key == "whisper_model"){
            app->selected_model = std::stoi(value);
            app->stream_data.selected_model = app->selected_model;
        }

        if (key == "input_device"){
            // Persisted by device name; the PortAudio index is resolved later
            // (in the settings tab / when recording starts) since indices are
            // not stable across runs and PortAudio isn't initialized yet here.
            app->selected_input_device_name = value;
        }

        if (key == "history_capacity"){
            app->history_buf->capacity = std::stoi(value);
        }

        if (key == "last_selected_deck"){
            app->last_selected_deck = value;
        }
    }
}

// Handles writing the file.
void save_config(const AppState* app){
    std::string executable_path = get_data_dir("JapaneseHelper").string();
    std::ofstream config(executable_path + "/config.ini");
    if (!config.is_open()){
        std::println("ERR | Error while SAVING config file!");
        return;
    }
    
    config << "deepl_key=" + app->deepl_key + '\n';
    config << "mymemory_email=" + app->mymemory_email + '\n';
    config << "whisper_model=" + std::to_string(app->selected_model) + '\n';
    config << "input_device=" + app->selected_input_device_name + '\n';
    config << "history_capacity=" + std::to_string(app->history_buf->capacity) + '\n';
    config << "last_selected_deck=" + app->last_selected_deck + '\n';

    capture_windows();
}

// A standalone function to clear the history buffer and file
// Not a real point to this since we have a button now
void clear_history(AppState* app){
    // Clear the circular buffer in memory
    CircularBuffer& buf = *app->history_buf;

    buf.data.clear();
    buf.time.clear();
    buf.api.clear();

    buf.head = 0;
    buf.tail = 0;
    buf.size = 0;

    std::string executable_path = get_data_dir("JapaneseHelper").string();
    std::ofstream history(executable_path + "/history.json");
    if (!history.is_open()){
        std::println("ERR | Error while WRITING to search history file!");
        return;
    }
    std::println("INFO | Cleared history.json file.");
}