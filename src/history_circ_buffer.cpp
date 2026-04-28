#include <print>
#include <fstream>

#include "json.hpp"

#include "include/app_state.h"
#include "include/get_exec_path.h"
#include "include/history_circ_buffer.h"

/* 
  Puts the given string into the circular buffer and also
  checks to see if the buffer is full

  Replaces the last inserted string if full

  Also, gets the time before putting into the buffer
*/
void enqueue(CircularBuffer& buf, const std::string& val){
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

    // Move the tail by one and go to 0 if buf.tail + 1 == buf.capacity
    buf.tail = (buf.tail + 1) % buf.capacity;
    buf.size++;

    std::println("INFO | Last search: {}", buf.data[buf.tail - 1]);
    std::println("INFO | Time of search: {}", buf.time[buf.tail - 1]);
    std::println("INFO | Current DATA buffer looks like: {}", buf.data);
    std::println("INFO | Current TIME buffer looks like: {}", buf.time);
    std::println("INFO | Head is at: {}", buf.head);
    std::println("INFO | Tail is at: {}", buf.tail);
}

/* 
  Puts the given string into the circular buffer and also
  checks to see if the buffer is full

  Replaces the last inserted string if full

  Also, instead of getting the time when writing to the buffer,
  uses the time given by the caller
*/
void enqueue(CircularBuffer& buf, const std::string& val, const std::string& time){
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

void load_buffer(CircularBuffer& buf){
    std::string executable_path = get_executable_path().parent_path().string();
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
    
    nlohmann::json j = nlohmann::json::parse(history);

    for (int i = 0; i < j["search"].size(); i++){
        enqueue(buf, j["search"][i], j["time"][i]);
    }
}

/* 
   This function is used to overwrite the history.json file in order to remain equivalent to the buffer
   The normal write function (enqueue_and_write) could go past the buffer capacity while also creating a large text file for no reason
   This ensures that the text file always remains in the boundaries of the buffer capacity
*/ 
void write_buffer(CircularBuffer& buf){
    nlohmann::json j;
    std::string executable_path = get_executable_path().parent_path().string();
    std::ofstream history(executable_path + "/history.json");
    if (!history.is_open()){
        std::println("ERR | Error while WRITING to search history file!");
        return;
    }
    std::println("INFO | Cleared history.json file.");

    for (int i = 0; i < buf.size; i++){
        j["search"].push_back(buf.data[(buf.head + i) % buf.capacity]);
        j["time"].push_back(buf.time[(buf.head + i) % buf.capacity]);
    }

    history << j.dump(4);
}

void print_buffers(CircularBuffer& buf){
    std::println("INFO | Current DATA buffer looks like: {}", buf.data);
    std::println("INFO | Current TIME buffer looks like: {}", buf.time);
}