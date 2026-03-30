#pragma once
#include <portaudio.h>
#include <string>
#include <vector>

#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define MAX_RECORDING_DURATION 30

struct StreamData {
    StreamData(){
        audio_samples.resize(SAMPLE_RATE * MAX_RECORDING_DURATION);
    }
    std::vector<float> audio_samples;
    bool is_recording = false;
    std::size_t index = 0;
};

int pa_callback(const void* in_buffer, void* out_buffer, unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                           void* user_data);
void create_stream(const PaStreamParameters* in_buffer, PaStreamParameters* out_buffer, StreamData* sd);
std::string whisper_transcribe(void* user_data);
void check_err(PaError err);
