#pragma once
#include <string>
#include <vector>

typedef struct CircularBuffer {
    std::vector<std::string> data;
    int head;
    int tail;
    int size;
    int capacity;
} CircularBuffer;

void enqueue(CircularBuffer& buf, const std::string& val);
void enqueue_and_write(CircularBuffer& buf, const std::string& val);
std::string dequeue(CircularBuffer& buf);
void load_buffer(CircularBuffer& buf);
void write_buffer(CircularBuffer& buf);