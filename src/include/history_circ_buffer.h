#pragma once
#include <string>
#include <vector>

// To log the time of search
#include <chrono>
#include <ctime>

struct AppState; // forward declare — breaks the cycle

typedef struct CircularBuffer {
    std::vector<std::string> data;
    std::vector<std::string> time;
    std::vector<int> api;
    int head = 0;
    int tail = 0;
    int size = 0;
    int capacity = 50;  // Used when config.ini has no valid history_capacity
} CircularBuffer;

void enqueue(AppState* app, const std::string& val);
void enqueue(AppState* app, const std::string& val, const std::string& time, const int api);    // uses provided time
void enqueue_and_write(CircularBuffer& buf, const std::string& val);
std::string dequeue(CircularBuffer& buf);
void load_buffer(AppState* app);
void write_buffer(AppState* app);
void print_buffers(AppState* app);
void resize_history_buffer(AppState* app, int new_capacity);