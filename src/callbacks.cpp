#include <exception>
#include <portaudio.h>
#include <thread>
#include <print>
#include <fstream>

#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>

#include "include/api.h"
#include "include/colors.h"
#include "include/callbacks.h"
#include "include/settings.h"
#include "include/app_state.h"
#include "include/get_exec_path.h"
#include "include/truncate_label.h"
#include "include/speech_to_text.h"
#include "include/history_circ_buffer.h"

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
            enqueue(app, word);
            // std::println("Last Search: {}", dequeue(*app->history_buf));
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(result.c_str());	// perform operation
            Fl::unlock();
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
            enqueue(app, word);
            // std::println("Last Search: {}", dequeue(*app->history_buf));
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

    // app->settings_win->hide();
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

// The callback when the history button is clicked
// Opens the history window and populates it with the search history from the circular buffer
void on_history_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    app->history_scroll->begin();
    app->history_scroll->clear(); // destroy existing child widgets inside the scroll

    int y = 10;
    const int history_width = 320;
    const int history_height_default = 500;
    const int history_height_empty = 50;

    if (app->history_buf->size == 0){
        app->history_win->size(history_width, history_height_empty);
        app->history_scroll->resize(0, 0, history_width, history_height_empty);

        Fl_Box* box = new Fl_Box(10, y, 280, 30, "No history yet!");
        box->align(FL_ALIGN_CENTER);
        box->labelfont(FL_ITALIC);
        box->labelcolor(FL_BLACK);
    }
    else{
        app->history_win->size(history_width, history_height_default);
        app->history_scroll->resize(0, 0, history_width, history_height_default);

        for (int i = app->history_buf->size - 1; i >= 0; i--){
            int idx = (app->history_buf->head + i) % app->history_buf->capacity;

            Fl_Button* btn = new Fl_Button(10, y, 280, 30, "");

            std::string query   = app->history_buf->data[idx];
            std::string date    = app->history_buf->time[idx];
            int api = app->history_buf->api[idx];
            auto* entry = new HistoryEntryData{app, query, api};
            btn->callback(on_history_entry_click, entry);

            std::string result  = truncate_label(query + "  —  " + date, btn->w() - 10);
            btn->copy_label(result.c_str());
            std::string tooltip = query + "  —  " + date;

            if (api == 0){
                tooltip += "  (Jisho)";
            } else if (api == 1){
                tooltip += "  (DeepL)";
            }

            btn->copy_tooltip(tooltip.c_str());
            y += 35;
        }
    }

    app->history_scroll->end();
    app->history_scroll->redraw();
    app->history_win->show();
    app->history_win->take_focus();
}

// The callback when the main window is closed
// Saves the history buffer into history.json before exiting
void on_main_win_close(Fl_Widget* w, void* data) {
    AppState* app = static_cast<AppState*>(data);
    write_buffer(app);
    w->hide();
}

// Callback for handling settings window tab changes
// This is triggered when any of the settings category buttons are clicked
void on_settings_win_change(Fl_Widget* w, void* data){
    AppState* app = static_cast<AppState*>(data);

    // Determine which button was pressed by comparing widget pointers
    if (w == app->general_settings_btn) {
        app->selected_settings_win = 0;
    } else if (w == app->history_settings_btn) {
        app->selected_settings_win = 1;
    } else if (w == app->api_settings_btn) {
        app->selected_settings_win = 2;
    } else if (w == app->stt_settings_btn) {
        app->selected_settings_win = 3;
    }

    std::println("INFO | Selected settings tab: {}", app->selected_settings_win);

    if (app->selected_settings_win == 0){
        app->settings_content->clear();
        app->settings_content->begin();
        
        // Display general settings

        app->settings_content->end();
        app->settings_win->redraw();
    } else if (app->selected_settings_win == 1){
        app->settings_content->clear();
        app->settings_content->begin();

        // Display history settings
        Fl_Button* clear_history_btn = new Fl_Button(340, 260, 160, 30, "Clear History");
        clear_history_btn->align(FL_ALIGN_CENTER);
        clear_history_btn->box(FL_UP_BOX);
        clear_history_btn->color(accent_blue);
        clear_history_btn->labelcolor(FL_WHITE);
        clear_history_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        clear_history_btn->callback(on_clear_history_btn, app);

        app->settings_content->end();
        app->settings_win->redraw();
    } else if (app->selected_settings_win == 2){
        app->settings_content->clear();
        app->settings_content->begin();

        // Display API settings
        Fl_Box* api_box = new Fl_Box(310, 10, 300, 30, "DeepL API Key:");
        app->settings_key_input = new Fl_Input(310, 10, 380, 30);
        api_box->align(FL_ALIGN_LEFT);
        api_box->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        
        
        app->settings_win->end();
        app->settings_win->redraw();
    } else if (app->selected_settings_win == 3){
        app->settings_content->clear();
        app->settings_content->begin();

        // Display STT settings
        Fl_Choice* model_selector = new Fl_Choice(0, 0, 80, 30, "Whisper Model:");

        app->settings_content->end();
        app->settings_win->redraw();
    }
}

// Clears the search history both in the buffer and in the history.json file
void on_clear_history_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);
    CircularBuffer* buf = app->history_buf;

    std::string executable_path = get_executable_path().parent_path().string();
    std::ofstream history(executable_path + "/history.json");
    if (!history.is_open()){
        std::println("ERR | Error while WRITING to search history file!");
        return;
    }
    std::println("INFO | Cleared history.json file.");

    buf->data.clear();
    buf->time.clear();
    buf->api.clear();
    buf->head = 0;
    buf->tail = 0;
    buf->size = 0;

    print_buffers(app);
}

// This callback is triggered when a history entry button is clicked
void on_history_entry_click(Fl_Widget* w, void* entry_data){
    auto* entry = static_cast<HistoryEntryData*>(entry_data);

    entry->app->input->value(entry->query.c_str());
    entry->app->selected_api = entry->api;

    if (entry->app->api_selector) {
        entry->app->api_selector->value(entry->api);
    }

    master_on_search(w, entry->app);
    entry->app->main_win->take_focus();

    delete entry;
}