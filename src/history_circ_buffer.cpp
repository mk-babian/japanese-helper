#include <print>
#include <fstream>

#include "include/app_state.h"
#include "include/get_exec_path.h"
#include "include/history_circ_buffer.h"

/* 
  Puts the given string into the circular buffer and also
  checks to see if the buffer is full

  Replaces the last inserted string if full
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
    // Move the tail by one and go to 0 if buf.tail + 1 == buf.capacity
    buf.tail = (buf.tail + 1) % buf.capacity;
    buf.size++;
    std::println("INFO | Current circular buffer looks like: {}", buf.data);
    std::println("INFO | Head is at: {}", buf.head);
    std::println("INFO | Tail is at: {}", buf.tail);
}

// Same as the function above, however writes to file after each search
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
    std::ifstream history(executable_path + "/history.txt");
    if (!history.is_open()){
        std::println("ERR | Error while READING from search history file!");
        return;
    }
    
    std::string line;
    while (std::getline(history, line)){
        enqueue(buf, line);
    }
}

/* 
   This function is used to overwrite the history.txt file in order to remain equivalent to the buffer
   The normal write function (enqueue_and_write) could go past the buffer capacity while also creating a large text file for no reason
   This ensures that the text file always remains in the boundaries of the buffer capacity
*/ 
void write_buffer(CircularBuffer& buf){
    std::string executable_path = get_executable_path().parent_path().string();
    std::ofstream history(executable_path + "/history.txt");
    if (!history.is_open()){
        std::println("ERR | Error while WRITING to search history file!");
        return;
    }
    std::println("INFO | Cleared history.txt file.");

    for (int i = 0; i < buf.size; i++){
        // Iterate from the head and wrap around
        // The modulus makes sure that we don't go over the capacity
        history << buf.data[(buf.head + i) % buf.capacity] << '\n';
    }
}