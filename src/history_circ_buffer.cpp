#include "include/app_state.h"
#include "include/history_circ_buffer.h"

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
}

std::string dequeue(CircularBuffer& buf){
    // Get the string at buf.head
    std::string val = buf.data[buf.head];
    // Move the head by 1
    buf.head = (buf.head + 1) % buf.capacity;
    buf.size--;
    return val;
}