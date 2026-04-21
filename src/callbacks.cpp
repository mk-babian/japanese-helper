#include <exception>
#include <portaudio.h>
#include <thread>
#include <print>

#include <FL/Fl_Choice.H>

#include "include/callbacks.h"
#include "include/api.h"
#include "include/app_state.h"
#include "include/settings.h"
#include "include/speech_to_text.h"
#include "include/colors.h"

void master_on_search(Fl_Widget* w, void* data){
    AppState* app = static_cast<AppState*>(data);
    app->search_btn->deactivate();
    app->search_btn->color(bg_color);

    std::string text = app->input->value();
    if (text.empty()){
        app->search_btn->activate();
        app->search_btn->color(accent_blue);
        return;
    }

    if (app->selected_api == 0){
        on_search_jisho(w, data);
    } else if (app->selected_api == 1){
        on_search_deepl(w, data);
    }
}

void on_search_jisho(Fl_Widget* w, void* data){
    (void)w;
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();

    // create new thread and give it copies of app and word
    std::thread([app, word](){
        // run at the same time as the main thread (that handles fltk)
        try {
            std::string result = jisho_lookup(word);
            enqueue(*app->history_buf, result);
            std::println("Last Search: {}", dequeue(*app->history_buf));
            Fl::lock();					        // lock mutex
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(result.c_str());	// perform operation
            Fl::unlock();				        // unlock mutex
            Fl::awake();				        // tell the main thread that smth changed
        } catch (const std::exception& e) {		// if jisho_lookup crashes, display error
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(e.what()); 	 	// show the error
            Fl::unlock();
            Fl::awake();
        }
    }).detach();        // let the thread take care of itself afterwards (keep the ui running)
}

void on_search_deepl(Fl_Widget* w, void* data){
    (void)w;
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();

    std::thread([app, word](){
        try {
            std::string result = deepl_translate(word, app->deepl_key);
            enqueue(*app->history_buf, result);
            std::println("Last Search: {}", dequeue(*app->history_buf));
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(result.c_str());
            Fl::unlock();
            Fl::awake();
        } catch (const std::exception& e) {
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(e.what());
            Fl::unlock();
            Fl::awake();
        }
    }).detach();
}

void choice_callback(Fl_Widget* w, void* data){
    Fl_Choice* choice = static_cast<Fl_Choice*>(w);
    AppState* app = static_cast<AppState*>(data);

    // okay, so choice->value values are as follows:
    // 0 for jisho
    // 1 for deepl
    // and so on...
    app->selected_api = choice->value();
}

void open_settings(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    app->settings_win->show();
}

void on_save_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data); 

    std::string key = app->settings_key_input->value(); 
    if (key.empty()) return;
    app->deepl_key = key;

    save_config(app);

    app->settings_win->hide();
}

void on_cancel_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);
    app->settings_win->hide();
}

void on_stt_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    // If already recording, signal to stop and return.
    // The existing background thread will catch this, stop PortAudio, and reset the UI.
    if (app->stream_data.is_recording){
        app->stream_data.is_recording = false;
        return;
    }

    // If not recording, set up UI immediately on the main thread.
    app->stream_data.is_recording = true;
    app->stream_data.index = 0;
    
    app->stt_btn->color(accent_red);
    app->stt_btn->selection_color(accent_red);   // Keep it red if clicked while recording.
    app->stt_btn->redraw();                         // Force FLTK to paint the town red.

    // Set up PortAudio parameters
    PaStreamParameters params;
    params.device = Pa_GetDefaultInputDevice();
    params.channelCount = 1;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = Pa_GetDeviceInfo(params.device)->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = NULL;

    // Spawn one thread to handle the blocking audio stream.
    std::thread([app, params](){
        try {
            // This function blocks until is_recording is set to false,
            // (via click) or buffer fills.
            create_stream(&params, NULL, &app->stream_data);

            // We transcribe now - audio is in stream_data, thread is still active 
            std::string result = whisper_transcribe(&app->stream_data);

            // Once the stream stops/finishes, reset the UI.
            Fl::lock();
            app->input->value(result.c_str());
            app->stt_btn->color(accent_blue);
            app->stt_btn->selection_color(accent_blue);
            app->stt_btn->redraw();                 // Force FLTK to paint the blue color now.
            app->stream_data.is_recording = false;
            Fl::unlock();
            Fl::awake();
        } catch (const std::exception& e){
            Fl::lock();
            app->stt_btn->color(accent_blue);
            app->stt_btn->selection_color(accent_blue);
            app->stt_btn->redraw();
            app->stream_data.is_recording = false;
            Fl::unlock();
            Fl::awake();
        }     
    }).detach();
}

void on_history_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);
    
    app->history_win->show();
}