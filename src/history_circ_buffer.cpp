#include <print>
#include <fstream>

#include "json.hpp"

#include "include/app_state.h"
#include "include/get_data_dir.h"
// i don't use this anymore ↓
#include "include/get_exec_path.h"
#include "include/history_circ_buffer.h"

/* 
  Puts the given string into the circular buffer and also
  checks to see if the buffer is full

  Replaces the last inserted string if full

  Also, gets the time before putting into the buffer
*/
void enqueue(AppState* app, const std::string& val){
    CircularBuffer& buf = *app->history_buf;

    // Only runs when ring is filled
    if (buf.size == buf.capacity){
        // This ensures that our head goes 1 over our previous head (overwrites)
        buf.head = (buf.head + 1) % buf.capacity;
        buf.size--;
    }
    // Assign value to the current tail
    buf.data[buf.tail] = val;

    // Get the current time
    auto now = std::chrono::system_clock::now();
    // Convert the current time to time_t
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    // Convert to local time struct
    std::tm* time = std::localtime(&t);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%A, %B %d %I:%M %p", time);
    std::string result(buffer);
    buf.time[buf.tail] = result;

    // Put the current used API into the buffer's vector of integers
    // This is useful if we ever want to search using our history again
    buf.api[buf.tail] = app->selected_api;

    // Move the tail by one and go to 0 if buf.tail + 1 == buf.capacity
    buf.tail = (buf.tail + 1) % buf.capacity;
    buf.size++;

    int inserted = (buf.tail - 1 + buf.capacity) % buf.capacity;
    std::println("INFO | Last search: {}", buf.data[inserted]);
    std::println("INFO | Time of search: {}", buf.time[inserted]);
    // std::println("INFO | Current DATA buffer looks like: {}", buf.data);
    // std::println("INFO | Current TIME buffer looks like: {}", buf.time);
    // std::println("INFO | Current API buffer looks like: {}", buf.api);
    // std::println("INFO | Head is at: {}", buf.head);
    // std::println("INFO | Tail is at: {}", buf.tail);
    // std::println("INFO | Size is: {}", buf.size);
}

/* 
  Puts the given string into the circular buffer and also
  checks to see if the buffer is full

  Replaces the last inserted string if full

  Also, instead of getting the time when writing to the buffer,
  uses the time given by the caller
*/
void enqueue(AppState* app, const std::string& val, const std::string& time, const int api){
    CircularBuffer& buf = *app->history_buf;

    // Only runs when ring is filled
    if (buf.size == buf.capacity){
        // This ensures that our head goes 1 over our previous head (overwrites)
        buf.head = (buf.head + 1) % buf.capacity;
        buf.size--;
    }
    // Assign value to the current tail
    buf.data[buf.tail] = val;

    // Assign the current time to tail
    buf.time[buf.tail] = time;

    // Put the current used API into the buffer's vector of integers
    buf.api[buf.tail] = api;

    // Move the tail by one and go to 0 if buf.tail + 1 == buf.capacity
    buf.tail = (buf.tail + 1) % buf.capacity;
    buf.size++;
}

// Same as the function above, however writes to file after each search
// DEPRACATED (I don't really use this anymore)
void enqueue_and_write(CircularBuffer& buf, const std::string& val){
    // Find the executable
    std::string executable_path = get_executable_path().parent_path().string();
    // Create and open history.txt to write search history into and also check for errors
    std::ofstream history(executable_path + "/history.txt", std::ios::app);
    if (!history.is_open()){
        std::println("ERR | Error while WRITING to search history file!");
        return;
    }

    // Only runs when ring is filled
    if (buf.size == buf.capacity){
        // This ensures that our head goes 1 over our previous head (overwrites)
        buf.head = (buf.head + 1) % buf.capacity;
        buf.size--;
    }
    // Assign value to the current tail
    buf.data[buf.tail] = val;
    if (val != "Input text here..."){
        history << val << '\n';
    }

    // Move the tail by one and go to 0 if buf.tail + 1 == buf.capacity
    buf.tail = (buf.tail + 1) % buf.capacity;
    buf.size++;
}

/* 
  Removes data from the buffer and moves head by 1
  Also, decrements size
*/
std::string dequeue(CircularBuffer& buf){
    // Get the string at buf.head
    std::string val = buf.data[buf.head];
    // Move the head by 1
    buf.head = (buf.head + 1) % buf.capacity;
    buf.size--;
    return val;
}

void load_buffer(AppState* app){
    std::string executable_path = get_data_dir("JapaneseHelper").string();
    std::ifstream history(executable_path + "/history.json");

    if (!history.is_open()){
        std::println("ERR | Error while READING from search history file!");
        return;
    }
    
    // Check if file is empty
    if (history.peek() == std::ifstream::traits_type::eof()){
        std::println("INFO | History file is empty, nothing to load");
        return;
    }

    // Check if first line is "null" (happens when file is empty but not actually empty)
    std::string first_line;
    std::getline(history, first_line);
    if (first_line == "null"){
        std::println("INFO | History file is empty, nothing to load");
        return;
    }
    
    history.seekg(0);   // Go back to the beginning of the file after checking for emptiness
    nlohmann::json j = nlohmann::json::parse(history);

    for (size_t i = 0; i < j["search"].size(); i++){
        enqueue(app, j["search"][i], j["time"][i], j["api"][i]);
    }
}

/* 
   This function is used to overwrite the history.json file in order to remain equivalent to the buffer
   The normal write function (enqueue_and_write) could go past the buffer capacity while also creating a large text file for no reason
   This ensures that the text file always remains in the boundaries of the buffer capacity
*/ 
void write_buffer(AppState* app){
    CircularBuffer& buf = *app->history_buf;

    nlohmann::json j;
    std::string executable_path = get_data_dir("JapaneseHelper").string();
    std::ofstream history(executable_path + "/history.json");
    if (!history.is_open()){
        std::println("ERR | Error while WRITING to search history file!");
        return;
    }
    std::println("INFO | Cleared history.json file.");

    for (int i = 0; i < buf.size; i++){
        j["search"].push_back(buf.data[(buf.head + i) % buf.capacity]);
        j["time"].push_back(buf.time[(buf.head + i) % buf.capacity]);
        j["api"].push_back(buf.api[(buf.head + i) % buf.capacity]);
    }

    history << j.dump(4);
}

void print_buffers(AppState* app){
    CircularBuffer& buf = *app->history_buf;
    std::println("INFO | Current DATA buffer looks like: {}", buf.data);
    std::println("INFO | Current TIME buffer looks like: {}", buf.time);
    std::println("INFO | Current API buffer looks like: {}", buf.api);
    std::println("INFO | Head is at: {}", buf.head);
    std::println("INFO | Tail is at: {}", buf.tail);
    std::println("INFO | Size is: {}", buf.size);
}