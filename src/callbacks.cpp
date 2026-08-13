#include <exception>
#include <portaudio.h>
#include <thread>
#include <print>
#include <fstream>
#include <cstdint>

#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Box.H>

#include "include/api.h"
#include "include/colors.h"
#include "include/callbacks.h"
#include "include/settings.h"
#include "include/app_state.h"
#include "include/get_exec_path.h"
#include "include/get_data_dir.h"
#include "include/truncate_label.h"
#include "include/speech_to_text.h"
#include "include/history_circ_buffer.h"
#include "include/download_whisper_model.h"

const std::filesystem::path executable_path = get_executable_path().parent_path();

// Resolve a saved input-device name to its current PortAudio index.
// Returns paNoDevice when the name is empty or no longer matches a device,
// in which case callers should fall back to the system default.
static PaDeviceIndex resolve_input_device(const std::string& name){
    if (name.empty()) return paNoDevice;

    int n_devices = Pa_GetDeviceCount();
    for (int i = 0; i < n_devices; i++){
        const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
        if (device_info == nullptr || device_info->maxInputChannels <= 0) continue;
        if (name == device_info->name) return i;
    }
    return paNoDevice;
}

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
    } else if (app->selected_api == 2){
        on_search_mymemory(w, data);
    }

    app->anki_button->show();
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
            app->anki_button->redraw();
            Fl::unlock();
            Fl::awake();				        // tell the main thread that smth changed
        } catch (const std::exception& e) {		// if jisho_lookup crashes, display error
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(e.what()); 	 	// show the error
            app->anki_button->redraw();
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
            app->anki_button->redraw();
            Fl::unlock();
            Fl::awake();
        } catch (const std::exception& e) {
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(e.what());
            app->anki_button->redraw();
            Fl::unlock();
            Fl::awake();
        }
    }).detach();
}

void on_search_mymemory(Fl_Widget* w, void* data){
    (void)w;
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();

    std::thread([app, word](){
        try {
            std::string result = mymemory_translate(word, app->mymemory_email);
            enqueue(app, word);
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(result.c_str());
            app->anki_button->redraw();
            Fl::unlock();
            Fl::awake();
        } catch (const std::exception& e) {
            Fl::lock();
            app->search_btn->activate();
            app->search_btn->color(accent_blue);
            app->output->value(e.what());
            app->anki_button->redraw();
            Fl::unlock();
            Fl::awake();
        }
    }).detach();
}

// The choice callback for the API Fl_Choice widget
void choice_callback(Fl_Widget* w, void* data){
    Fl_Choice* choice = static_cast<Fl_Choice*>(w);
    AppState* app = static_cast<AppState*>(data);

    // okay, so choice->value values are as follows:
    // 0 for jisho
    // 1 for deepl
    // and so on...
    app->selected_api = choice->value();
}

// The callback for opening the setting window
void open_settings(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    // Open on the General tab.
    on_settings_win_change(app->general_settings_btn, app);

    app->settings_win->show();
}

void open_info(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    // Open on the API tab.
    on_info_win_change(app->general_info_btn, app);

    app->info_win->show();
}

// Callback for handling info window tab changes
// This is triggered when any of the info category buttons are clicked
void on_info_win_change(Fl_Widget* w, void* data){
    AppState* app = static_cast<AppState*>(data);

    app->info_text = nullptr;

    app->info_content->clear();

    // Determine which button was pressed by comparing widget pointers
    if (w == app->api_info_btn) {
        app->selected_info_win = 0;
    }else if (w == app->general_info_btn) {
        app->selected_info_win = 1;
    }else if (w == app->whisper_info_btn) {
        app->selected_info_win = 2;
    }

    std::println("INFO | Selected info tab: {}", app->selected_info_win);

    if (app->selected_info_win == 0){
        app->info_content->begin();

        // Display API information
        app->info_text = new Fl_Multiline_Output(210, 10, 280, 580);
        app->info_text->box(FL_NO_BOX);
        app->info_text->color(FL_WHITE);
        app->info_text->textfont((Fl_Font)(FL_FREE_FONT + 1));
        app->info_text->textcolor(FL_BLACK);
        app->info_text->wrap(1);
        app->info_text->value(
            "\t  === Jisho ===\n\n"
            "A Japanese-English dictionary. Looks up words, "
            "readings, and definitions. No API key required.\n\n"
            "Works with Kanji, Hiragana, Katakana, and Romaji input.\n"
            "\n"
            "\t  === DeepL ===\n\n"
            "A machine translation service. Requires a free API "
            "key, set it in Settings > API.\n\n"
            "Way more accurate than MyMemory, especially for sentences.\n"
            "\n"
            "\t === MyMemory ===\n\n"
            "A translation memory service. No API key required. "
            "Providing an email in Settings > API raises the free "
            "daily limit from 5,000 to 50,000 characters."
        );

        app->info_content->end();
        app->info_win->redraw();
    } else if (app->selected_info_win == 1){
        app->info_content->begin();

        // Display general information
        app->info_text = new Fl_Multiline_Output(210, 10, 280, 580);
        app->info_text->box(FL_NO_BOX);
        app->info_text->color(FL_WHITE);
        app->info_text->textfont((Fl_Font)(FL_FREE_FONT + 1));
        app->info_text->textcolor(FL_BLACK);
        app->info_text->wrap(1);
        app->info_text->value(
            "A simple lookup and translation desktop app for Japanese. Built with C++ and FLTK.\n\n"
        );
        app->info_content->end();
        app->info_win->redraw();
    } else if (app->selected_info_win == 2){
        app->info_content->begin();

        // Display Whisper information
        app->info_text = new Fl_Multiline_Output(210, 10, 280, 580);
        app->info_text->box(FL_NO_BOX);
        app->info_text->color(FL_WHITE);
        app->info_text->textfont((Fl_Font)(FL_FREE_FONT + 1));
        app->info_text->textcolor(FL_BLACK);
        app->info_text->wrap(1);
        app->info_text->value(
            "Whisper is a high-performance inference of OpenAI's Whisper automatic speech recognition (ASR) model.\n\n"
            "To get started, please download a model from Settings → Speech-to-text → Whisper Model.\n\n"
            "Do note that the accuracy, speed, and hardware usage varies from model to model.\n\n"
            "Here is a chart of all the available models. I recomend starting with the \"small\" model.\n\n"
            "| Model  | Disk    | Mem     |\n"
            "| ------ | ------- | ------- |\n"
            "| tiny   | 75 MiB  | ~273 MB |\n"
            "| base   | 142 MiB | ~388 MB |\n"
            "| small  | 466 MiB | ~852 MB |\n"
            "| medium | 1.5 GiB | ~2.1 GB |\n"
            "| large  | 2.9 GiB | ~3.9 GB |\n\n"
            "After doing so, click on the microphone button next to the search bar, record your input, "
            "and press it again to stop recording."
        );
        app->info_content->end();
        app->info_win->redraw();
    }
}

// The apply button found in the settings window, its callback
void on_apply_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data); 

    if (app->settings_key_input){
        std::string key = app->settings_key_input->value();
        if (!key.empty()) app->deepl_key = key;
    }

    // Empty is fine here; it just clears the saved email.
    if (app->settings_email_input){
        app->mymemory_email = app->settings_email_input->value();
    }

    if (app->history_capacity_input){
        app->history_buf->capacity = std::stoi(app->history_capacity_input->value());
        std::println("Capacity {}", app->history_buf->capacity);
    }

    save_config(app);

    // This crashes the app for some reason
    // Probably some dangling pointers or some-such
    // I guess we'll just NOT hide the window
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

    // Set up PortAudio parameters.
    // Resolve the saved device by name (indices aren't stable across runs),
    // falling back to the system default when it's gone or none was chosen.
    PaDeviceIndex device = resolve_input_device(app->selected_input_device_name);
    if (device == paNoDevice) device = Pa_GetDefaultInputDevice();
    app->selected_input_device = device;
    app->stream_data.selected_input_device = device;

    PaStreamParameters params;
    params.device = device;
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
            } else if (api == 2){
                tooltip += "  (MyMemory)";
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
    app->history_capacity = app->history_buf->capacity;
    write_buffer(app);
    save_config(app);
    w->hide();
}

// Callback for handling settings window tab changes
// This is triggered when any of the settings category buttons are clicked
void on_settings_win_change(Fl_Widget* w, void* data){
    AppState* app = static_cast<AppState*>(data);

    app->settings_key_input = nullptr;
    app->settings_email_input = nullptr;
    app->whisper_model_selector = nullptr;
    app->install_whisper_model = nullptr;
    app->whisper_device_selector = nullptr;

    app->settings_content->clear();

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
        app->settings_content->begin();
        
        // Display general settings
        

        app->settings_content->end();
        app->settings_win->redraw();
    } else if (app->selected_settings_win == 1){
        app->settings_content->begin();

        // Display history settings
        Fl_Button* clear_history_btn = new Fl_Button(535, 515, 160, 30, "Clear History");
        clear_history_btn->align(FL_ALIGN_CENTER);
        clear_history_btn->box(FL_UP_BOX);
        clear_history_btn->color(accent_red);
        clear_history_btn->labelcolor(FL_WHITE);
        clear_history_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        clear_history_btn->callback(on_clear_history_btn, app);

        Fl_Int_Input* history_capacity_input = new Fl_Int_Input(330, 10, 335, 30, "History Capacity:");
        history_capacity_input->box(FL_UP_BOX);
        history_capacity_input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        history_capacity_input->value(app->history_buf->capacity);
        app->history_capacity_input = history_capacity_input;

        app->settings_content->end();
        app->settings_win->redraw();
    } else if (app->selected_settings_win == 2){
        app->settings_content->begin();

        app->key_shown = false;

        // Display API settings
        app->settings_key_input = new Fl_Input(310, 10, 350, 30, "DeepL API Key:");
        app->settings_key_input->value(app->deepl_key.c_str());

        if (app->deepl_key.empty()) {
            app->settings_key_input->type(FL_NORMAL_INPUT);
            app->key_shown = true;
        } else {
            app->settings_key_input->type(FL_SECRET_INPUT);
            app->key_shown = false;
        }

        app->settings_key_input->box(FL_UP_BOX);
        app->settings_key_input->labelfont((Fl_Font)(FL_FREE_FONT + 1));

        Fl_Button* show_btn = new Fl_Button(665, 10, 30, 30);
        show_btn->color(accent_blue);
        show_btn->box(FL_UP_BOX);
        Fl_PNG_Image* show_icon = new Fl_PNG_Image((executable_path.string() + "/images/show.png").c_str());
        if (show_icon->fail()){
            std::println("W | Couldn't load show-icon image!");
        }else{
            show_btn->image(show_icon);
        }
        show_btn->callback(show_deepl_key_btn, app);

        // Optional email for MyMemory; raises the daily limit from 5,000 to 50,000 chars
        app->settings_email_input = new Fl_Input(310, 50, 350, 30, "MyMemory Email:");
        app->settings_email_input->value(app->mymemory_email.c_str());
        app->settings_email_input->box(FL_UP_BOX);
        app->settings_email_input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        app->settings_email_input->tooltip("Optional. Giving MyMemory a valid email raises the free daily limit from 5,000 to 50,000 characters.");

        app->settings_content->end();
        app->settings_win->redraw();
    } else if (app->selected_settings_win == 3){
        app->settings_content->begin();

        // Display STT settings
        app->whisper_model_selector = new Fl_Choice(310, 10, 80, 30, "Whisper Model:");
        app->whisper_model_selector->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        app->whisper_model_selector->add("tiny");
        app->whisper_model_selector->add("base");
        app->whisper_model_selector->add("small");
        app->whisper_model_selector->add("medium");
        app->whisper_model_selector->add("large");
        app->whisper_model_selector->callback(model_choice_callback, app);
        app->whisper_model_selector->value(app->selected_model);

        app->install_whisper_model = new Fl_Button(400, 10, 290, 30, "Download");
        app->install_whisper_model->box(FL_UP_BOX);
        app->install_whisper_model->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        app->install_whisper_model->labelcolor(FL_WHITE);
        app->install_whisper_model->color(accent_blue);
        app->install_whisper_model->callback(download_button, app);
        
        // Check if the current selected model is already downloaded
        
        std::string model_name = "";
        switch(app->selected_model){
            case 0: model_name = "ggml-tiny.bin";   break;
            case 1: model_name = "ggml-base.bin";   break;
            case 2: model_name = "ggml-small.bin";  break;
            case 3: model_name = "ggml-medium.bin"; break;
            case 4: model_name = "ggml-large.bin";  break;
        }
        if (std::filesystem::exists(executable_path / "whisper.cpp" / "models" / model_name)){
            app->install_whisper_model->color(accent_green);
            app->install_whisper_model->labelcolor(FL_BLACK);
            app->install_whisper_model->label("Already Downloaded!");
            app->install_whisper_model->deactivate();
        }

        // Input device selector: list every PortAudio device that has input channels.
        app->whisper_device_selector = new Fl_Choice(310, 50, 380, 30, "Input Device:");
        app->whisper_device_selector->labelfont((Fl_Font)(FL_FREE_FONT + 1));
        app->whisper_device_selector->callback(device_choice_callback, app);

        int n_devices = Pa_GetDeviceCount();
        PaDeviceIndex default_device = Pa_GetDefaultInputDevice();
        int selected_menu_index = -1;   // entry matching the saved device name
        int default_menu_index = -1;    // entry for the system default device
        for (int i = 0; i < n_devices; i++){
            const PaDeviceInfo* device_info = Pa_GetDeviceInfo(i);
            if (device_info == nullptr || device_info->maxInputChannels <= 0) continue;

            std::string device_name = device_info->name;

            // FLTK's add() treats '/' as a submenu separator and '&' as a shortcut
            // marker. Device names routinely contain both, so escape them with '\'.
            std::string label;
            for (char c : device_name){
                if (c == '/' || c == '&' || c == '\\') label += '\\';
                label += c;
            }
            // Flag the system default device right in the label.
            if (i == default_device) label += "  ( Default )";

            // Stash the PortAudio device index inside the menu item's user data so the
            // callback can recover it regardless of which devices were skipped above.
            app->whisper_device_selector->add(label.c_str(), 0,
                                              nullptr, (void*)(intptr_t)i);
            int menu_index = app->whisper_device_selector->size() - 2; // -1 for trailing NULL terminator

            // Render the default device's entry in bold.
            if (i == default_device){
                default_menu_index = menu_index;
                const_cast<Fl_Menu_Item*>(&app->whisper_device_selector->menu()[menu_index])
                    ->labelfont((Fl_Font)(FL_FREE_FONT + 1));
            }

            // Match the saved device by name; indices aren't stable across runs.
            if (!app->selected_input_device_name.empty() &&
                device_name == app->selected_input_device_name){
                selected_menu_index = menu_index;
                app->selected_input_device = i;
                app->stream_data.selected_input_device = i;
            }
        }

        // Pre-select the saved device if it's still present, otherwise the default.
        if (selected_menu_index >= 0){
            app->whisper_device_selector->value(selected_menu_index);
        } else if (default_menu_index >= 0){
            app->whisper_device_selector->value(default_menu_index);
            // The saved device is gone (or none saved); fall back to the default.
            app->selected_input_device = paNoDevice;
            app->stream_data.selected_input_device = paNoDevice;
        }

        app->settings_content->end();
        app->settings_win->redraw();
    }
}

// Clears the search history both in the buffer and in the history.json file
void on_clear_history_btn(Fl_Widget* w, void* data){
    AppState* app = static_cast<AppState*>(data);
    CircularBuffer* buf = app->history_buf;

    std::string path = get_data_dir("JapaneseHelper").string();
    std::ofstream history(path + "/history.json");
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

    // Give the user some visual confirmation that the history was cleared.
    Fl_Button* btn = static_cast<Fl_Button*>(w);
    btn->color(accent_green);
    btn->labelcolor(FL_BLACK);
    btn->label("History Cleared!");
    btn->redraw();
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

void model_choice_callback(Fl_Widget* w, void* data){
    Fl_Choice* choice = static_cast<Fl_Choice*>(w);
    AppState* app = static_cast<AppState*>(data);

    app->selected_model = choice->value();
    app->stream_data.selected_model = app->selected_model;

    // Check if the current selected model is already downloaded
    std::string model_name = "";
    switch(app->selected_model){
        case 0: model_name = "ggml-tiny.bin";   break;
        case 1: model_name = "ggml-base.bin";   break;
        case 2: model_name = "ggml-small.bin";  break;
        case 3: model_name = "ggml-medium.bin"; break;
        case 4: model_name = "ggml-large.bin";  break;
    }
    if (std::filesystem::exists(executable_path / "whisper.cpp" / "models" / model_name)){
        app->install_whisper_model->color(fl_rgb_color(54, 192, 96));
        app->install_whisper_model->labelcolor(FL_BLACK);
        app->install_whisper_model->label("Already Downloaded!");
        app->install_whisper_model->deactivate();
    }else{
        app->install_whisper_model->color(accent_blue);
        app->install_whisper_model->labelcolor(FL_WHITE);
        app->install_whisper_model->label("Download");
        app->install_whisper_model->activate();
    }

    std::println("INFO | Selected MODEL index: {}", app->selected_model);
}

void device_choice_callback(Fl_Widget* w, void* data){
    Fl_Choice* choice = static_cast<Fl_Choice*>(w);
    AppState* app = static_cast<AppState*>(data);

    // Recover the PortAudio device index we stashed in the menu item's user data.
    const Fl_Menu_Item* item = choice->mvalue();
    if (item == nullptr) return;

    PaDeviceIndex device = (PaDeviceIndex)(intptr_t)item->user_data();
    app->selected_input_device = device;
    app->stream_data.selected_input_device = device;

    // Persist by name, not index, since indices aren't stable across runs.
    const PaDeviceInfo* device_info = Pa_GetDeviceInfo(device);
    app->selected_input_device_name = (device_info != nullptr) ? device_info->name : "";

    std::println("INFO | Selected input device: {} (index {})",
                 app->selected_input_device_name, device);
}

void download_button(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    // Guard: make sure a model is actually selected
    if (app->selected_model < 0){
        app->install_whisper_model->label("Select model!");
        app->install_whisper_model->redraw();
        return;
    }

    // Disable the button while downloading so it can't be double-clicked
    app->install_whisper_model->deactivate();
    app->install_whisper_model->label("Downloading...");
    app->install_whisper_model->redraw();

    int model_index = app->selected_model;

    std::thread([app, model_index](){
        try {
            download_whisper_model(model_index);

            Fl::lock();
            app->install_whisper_model->activate();
            app->install_whisper_model->label("Done!");
            app->install_whisper_model->redraw();
            Fl::unlock();
            Fl::awake();
        } catch (const std::exception& e){
            Fl::lock();
            app->install_whisper_model->activate();
            app->install_whisper_model->label("Failed!");
            app->install_whisper_model->copy_tooltip(e.what());
            app->install_whisper_model->redraw();
            Fl::unlock();
            Fl::awake();
        }
    }).detach();
}

void show_deepl_key_btn(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    if (!app->settings_key_input) return;

    if (app->key_shown == false){
        // Reveal the key
        app->settings_key_input->type(FL_NORMAL_INPUT);
        app->settings_key_input->redraw();
        app->key_shown = true;
    } else {
        // Hide the key again
        app->settings_key_input->type(FL_SECRET_INPUT);
        app->settings_key_input->redraw();
        app->key_shown = false;
    }
}

void on_anki_button(Fl_Widget* w, void* data){
    (void)w;
    AppState* app = static_cast<AppState*>(data);

    anki_connect();
}