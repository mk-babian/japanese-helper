#include <portaudio.h>
#include <stdexcept>
#include <string>

#include "whisper.cpp/include/whisper.h"
#include "include/app_state.h"

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

    whisper_context_params w_context_params = whisper_context_default_params();
    struct whisper_context* ctx = whisper_init_from_file_with_params(
                "whisper.cpp/models/ggml-base.bin", w_context_params);
    if (ctx == nullptr) throw std::runtime_error("Failed to load model file for Whisper.cpp\n");

    whisper_full_params w_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY); 
    w_params.language = "ja";

    if (whisper_full(ctx, w_params, sd->audio_samples.data(), (int)sd->audio_samples.size()) != 0){
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
