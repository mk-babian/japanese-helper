#include <portaudio.h>
#if defined(_WIN32)
#include <pa_win_wasapi.h>
#endif
#include <stdexcept>
#include <string>
#include <print>

#include "whisper.cpp/include/whisper.h"
#include "include/app_state.h"
#include "include/get_exec_path.h"

/*
 * PortAudio calls this function automatically, we just create it.
 *
 * Arguments:
 *      in_buffer - raw pointer to audio data PA just captured;
 *      It's void* since PA doesn't know the format, we cast it to float*.
 *
 *      out_buffer - same as in_buffer, but for output.
 *      Not useful to us, hence we cast it to void.
 *
 *      frames_per_buffer - dictates how many audio frames are in this batch.
 *
 *      time_info - timestamp for when the I/O buffers were captured/played.
 *      Not useful to us, hence we cast it to void.
 *
 *      status_flags - tells if something went wrong in the stream.
 *      (buffer overflow/underflow). Cast to void.
 *
 *      user_data - the one pointer that we control.
 *      PA passes back whatever you gave it when you opened the stream.
 *      It holds our StreamData*, which holds our recording buffer and state.
 *
 *  Return values:
 *      paContinue - keep going, keep capturing.
 *      paComplete - stop calling the callback.
*/
int pa_callback(const void* in_buffer, void* out_buffer, unsigned long frames_per_buffer,
                           const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                           void* user_data){
    // Create pointer to StreamData of user_data that is cast to StreamData*.
    StreamData* sd = (StreamData*)user_data;
    // Check if the user hasn't deactivated recording (pressed button).
    // Continue if true.
    if (!sd->is_recording) return paContinue;
    
    // Cast all the unnecessary stuff to void.
    (void)out_buffer;
    (void)time_info;
    (void)status_flags;
    // Cast the input buffer to float*.
    float* in = (float*)in_buffer;

    // Copies current chunk into bigger recording buffer, advancing index each time.
    // Each callback call appends one chunk. After enough callbacks, audio_samples
    // is full and recording has been fully captured.
    for (std::size_t i = 0; i < frames_per_buffer; i++){
        sd->audio_samples[sd->index] = in[i];
        sd->index++;
    }

    // Check if the index is over the size of the audio_samples (meaning it's full).
    if (sd->index >= sd->audio_samples.size()) return paComplete;
    // Call the function again.
    return paContinue;
}

void create_stream(const PaStreamParameters* in_buffer, PaStreamParameters* out_buffer, StreamData* sd){
    // Create a handle pointer to be later passed into Pa_OpenStream.
    PaStream* stream;

    /* Pa_OpenStream opens a PA stream with the given arguments.
     *
     * Arguments:
     *      &stream - where to write the PA handle.
     *      in_buffer - PaStreamParameters* describing input device.
     *      out_buffer - same as input, but for output.
     *      SAMPLE_RATE - samples per second. Must match the transcription API.
     *      FRAMES_PER_BUFFER - chunk size per callback invocation.
     *      paNoFlag - no special flags.
     *      pa_callback - function pointer.
     *      sd - StreamData* passed back as user_data every callback.
    */
    PaError err = Pa_OpenStream(&stream, in_buffer, out_buffer, SAMPLE_RATE, FRAMES_PER_BUFFER,
                                paNoFlag, pa_callback, sd);
    // Call check_err on err.
    check_err(err);

    // Start the stream and check for errors again.
    err = Pa_StartStream(stream);
    check_err(err);

    // Parks the main thread in a 100ms polling loop until buffer is full or
    // is_recording is set to false (user clicks button).
    while (sd->index < sd->audio_samples.size() && sd->is_recording) Pa_Sleep(100);
    
    // Drains the stream. Waits for callbacks to finish, then deactivates.
    // + check for errors.
    err = Pa_StopStream(stream);
    check_err(err);

    // Release the hardware and free the stream object.
    err = Pa_CloseStream(stream);
    check_err(err);
}

std::string whisper_transcribe(void* user_data){
    StreamData* sd = (StreamData*)user_data;
    std::string res;

    std::string model_name = "";
        switch(sd->selected_model){
            case 0: model_name = "ggml-tiny.bin";   break;
            case 1: model_name = "ggml-base.bin";   break;
            case 2: model_name = "ggml-small.bin";  break;
            case 3: model_name = "ggml-medium.bin"; break;
            case 4: model_name = "ggml-large.bin";  break;
    }
    
    std::string path = get_executable_path().parent_path().string();
    std::string model_path = path + "/whisper.cpp/models/" + model_name;
    std::println("INFO | Loading model from: {}", model_path);

    whisper_context_params w_context_params = whisper_context_default_params();

    whisper_full_params w_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    w_params.language          = "ja";
    w_params.no_context        = true;   // don't carry context between segments — helps with short clips
    w_params.single_segment    = false;  // let it segment naturally
    w_params.print_special     = false;  // suppress [BLANK_AUDIO] etc. tokens in output
    w_params.suppress_blank    = true;   // suppress blank outputs between segments
    w_params.token_timestamps  = false;  // small perf gain if you don't need them
    w_params.temperature       = 0.0f;   // greedy is already 0, but be explicit

    struct whisper_context* ctx = whisper_init_from_file_with_params(
                model_path.c_str(), w_context_params);
    if (ctx == nullptr) throw std::runtime_error("Failed to load model file for Whisper.cpp\n");

    if (whisper_full(ctx, w_params, sd->audio_samples.data(), (int)sd->index) != 0){
        throw std::runtime_error("Whisper.cpp error\n");
        whisper_free(ctx);
        return res;
    }

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; i++){
        res += whisper_full_get_segment_text(ctx, i);
    }

    whisper_free(ctx);
    return res;
}

// Function that checks for errors.
void check_err(PaError err){
    if (err != paNoError){
        std::string error_text = std::string(Pa_GetErrorText(err));
        throw std::runtime_error("PortAudio error:\n" + error_text); 
    }
}

int rolling_callback(const void* in_buffer, void* out_buffer, unsigned long frames_per_buffer, 
                            const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                            void* user_data){
    // Create pointer to RollingStreamData of user_data that is cast to RollingStreamData*.
    RollingStreamData* rsd = (RollingStreamData*)user_data;

    // Cast all the unnecessary stuff to void.
    (void)out_buffer;
    (void)time_info;
    (void)status_flags;
    // Cast the input buffer to float*.
    float* in = (float*)in_buffer;

    // Guard against a ring buffer that hasn't been sized yet (rolling_start resizes it).
    if (rsd->ring.empty()) return paContinue;

    // Lock while writing so a concurrent rolling_snapshot() doesn't read a
    // half-updated ring buffer.
    std::lock_guard<std::mutex> lock(rsd->mtx);

    // Write each frame into the ring at the current write position, wrapping
    // around once we hit the end. Oldest audio gets overwritten automatically,
    // which is exactly the semantics we want for a rolling capture buffer.
    for (std::size_t i = 0; i < frames_per_buffer; i++){
        rsd->ring[rsd->write_pos] = in[i];
        rsd->write_pos = (rsd->write_pos + 1) % rsd->ring.size();
    }

    // Rolling capture is persistent: never signal completion, always continue.
    return paContinue;
}

// Opens a persistent WASAPI loopback capture stream on the given render device.
//
// Loopback is implicit in PortAudio: we open an *input* stream whose device is a
// render (output) device, and PortAudio detects that it should use
// AUDCLNT_STREAMFLAGS_LOOPBACK internally. The stream stays open until
// rolling_stop() is called, continuously feeding rolling_callback().
void rolling_start(RollingStreamData* data, PaDeviceIndex device_index){
#if defined(_WIN32)
    // Resolve "use the default" (paNoDevice) into a concrete device index.
    PaDeviceIndex dev = device_index;
    if (dev == paNoDevice){
        dev = Pa_GetDefaultOutputDevice();
    }

    // No render device at all -> nothing to loop back from.
    if (dev == paNoDevice){
        std::println("W | Rolling capture unavailable: no default output device.");
        data->running = false;
        return;
    }

    const PaDeviceInfo* dev_info = Pa_GetDeviceInfo(dev);
    if (dev_info == nullptr){
        std::println("W | Rolling capture unavailable: device {} has no info.", (int)dev);
        data->running = false;
        return;
    }

    // Loopback is a WASAPI-only feature. Other host APIs (MME, DirectSound, ...)
    // cannot open an input stream on an output device, so bail out gracefully.
    const PaHostApiInfo* host_info = Pa_GetHostApiInfo(dev_info->hostApi);
    if (host_info == nullptr || host_info->type != paWASAPI){
        std::println("W | Rolling capture unavailable: '{}' is not a WASAPI device.",
                     dev_info->name);
        data->running = false;
        return;
    }

    // Size the ring buffer up front. SAMPLE_RATE * ROLLING_BUFFER_SECONDS floats
    // holds 15 seconds of 16 kHz mono audio, overwriting the oldest once full.
    data->ring.assign((std::size_t)SAMPLE_RATE * ROLLING_BUFFER_SECONDS, 0.0f);
    data->write_pos = 0;
    data->selected_device = dev;

    // WASAPI stream config. paWinWasapiAutoConvert lets the engine insert a
    // system-level sample-rate/channel converter so we can request 16 kHz mono
    // even though the device's native mix format is typically 48 kHz stereo.
    PaWasapiStreamInfo wasapi_info = {};
    wasapi_info.size        = sizeof(PaWasapiStreamInfo);
    wasapi_info.hostApiType = paWASAPI;
    wasapi_info.version     = 1;
    wasapi_info.flags       = paWinWasapiAutoConvert;

    // Input parameters describing the (output) device we want to capture from.
    PaStreamParameters input_params = {};
    input_params.device                    = dev;
    input_params.channelCount              = 1;
    input_params.sampleFormat              = paFloat32;
    input_params.suggestedLatency          = dev_info->defaultLowOutputLatency;
    input_params.hostApiSpecificStreamInfo = &wasapi_info;

    // Open the stream. Input on an output device -> WASAPI loopback.
    PaError err = Pa_OpenStream(&data->stream, &input_params, nullptr, SAMPLE_RATE,
                                FRAMES_PER_BUFFER, paNoFlag, rolling_callback, data);
    if (err != paNoError){
        std::println("W | Rolling capture failed to open: {}", Pa_GetErrorText(err));
        data->stream = nullptr;
        data->running = false;
        return;
    }

    // Start capturing. The stream stays open until rolling_stop() is called.
    err = Pa_StartStream(data->stream);
    if (err != paNoError){
        std::println("W | Rolling capture failed to start: {}", Pa_GetErrorText(err));
        Pa_CloseStream(data->stream);
        data->stream = nullptr;
        data->running = false;
        return;
    }

    data->running = true;
    std::println("INFO | Rolling capture started on '{}'.", dev_info->name);
#else
    // WASAPI loopback is Windows-only; nothing to do on other platforms.
    (void)data;
    (void)device_index;
#endif
}

// Stops and closes the rolling capture stream. Call before Pa_Terminate() so the
// stream doesn't outlive PortAudio itself.
void rolling_stop(RollingStreamData* data){
    if (data->stream != nullptr){
        Pa_StopStream(data->stream);
        Pa_CloseStream(data->stream);
        data->stream = nullptr;
    }
    data->running = false;
}

// Returns a linear copy of the ring buffer in chronological order (oldest -> newest).
// Takes the mutex so it can't read a half-updated buffer while the callback writes.
std::vector<float> rolling_snapshot(RollingStreamData* data){
    std::lock_guard<std::mutex> lock(data->mtx);

    // The sample at write_pos is the oldest; the one just before it is the newest.
    // Reading forward from write_pos (wrapping) yields oldest -> newest order.
    std::vector<float> snapshot(data->ring.size());
    for (std::size_t i = 0; i < data->ring.size(); i++){
        snapshot[i] = data->ring[(data->write_pos + i) % data->ring.size()];
    }
    return snapshot;
}