#pragma once
#include <portaudio.h>
#include <mutex>
#include <string>
#include <vector>

#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define MAX_RECORDING_DURATION 30

#define ROLLING_BUFFER_SECONDS 15

struct StreamData {
    StreamData(){
        audio_samples.resize(SAMPLE_RATE * MAX_RECORDING_DURATION);
    }
    // The buffer we use is linear
    std::vector<float> audio_samples;
    bool is_recording = false;
    std::size_t index = 0;
    int selected_model = 0;
    // PortAudio device index to record from. paNoDevice means "use the default".
    PaDeviceIndex selected_input_device = paNoDevice;
};

/*struct RollingStreamData {
    // Buffer - Circular/ring buffer, overwrite oldest
    // Index - Head that wraps around
    // Lifecycle flag - "running" (persistent) + separate "snapshot now" trigger
    // Lifetime - Open once, stays open
    // Mutex required
};*/

struct RollingStreamData {
    std::vector<float> ring;   // SAMPLE_RATE * ROLLING_BUFFER_SECONDS floats
    std::size_t write_pos = 0;
    bool running = false;
    PaStream* stream = nullptr;
    std::mutex mtx;
    int selected_model = 0;
    PaDeviceIndex selected_device = paNoDevice;
};

int pa_callback(const void* in_buffer, void* out_buffer, unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                           void* user_data);
void create_stream(const PaStreamParameters* in_buffer, PaStreamParameters* out_buffer, StreamData* sd);
std::string whisper_transcribe(void* user_data);
void check_err(PaError err);
int rolling_callback(const void* in_buffer, void* out_buffer, unsigned long frames_per_buffer, 
                            const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                            void* user_data);
void rolling_start(RollingStreamData* data, PaDeviceIndex device_index);
void rolling_stop(RollingStreamData* data);
std::vector<float> rolling_snapshot(RollingStreamData* data);