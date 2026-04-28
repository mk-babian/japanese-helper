#pragma once
#include <string>
#include <vector>

// To log the time of search
#include <chrono>
#include <ctime>

typedef struct CircularBuffer {
    std::vector<std::string> data;
    std::vector<std::string> time;
    int head;
    int tail;
    int size;
    int capacity;
} CircularBuffer;

void enqueue(CircularBuffer& buf, const std::string& val);                             // generates time now
void enqueue(CircularBuffer& buf, const std::string& val, const std::string& time);    // uses provided time
void enqueue_and_write(CircularBuffer& buf, const std::string& val);
std::string dequeue(CircularBuffer& buf);
void load_buffer(CircularBuffer& buf);
void write_buffer(CircularBuffer& buf);
void print_buffers(CircularBuffer& buf);