#include <thread>

#include <Fl/Fl_Choice.H>

#include "include/callbacks.h"
#include "include/api.h"
#include "include/api_key.h"
#include "include/app_state.h"

void master_on_search(Fl_Widget* w, void* data){
    AppState* app = static_cast<AppState*>(data);

    if (app->selected_api == 0){
        on_search_jisho(w, data);
    } else if (app->selected_api == 1){
        on_search_deepl(w, data);
    }
}

void on_search_jisho(Fl_Widget*, void* data){
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();
    if (word.empty()) return;

    // create new thread and give it copies of app and word
    std::thread([app, word](){
        // run at the same time as the main thread (that handles fltk)
        try {
            std::string result = jisho_lookup(word);
            Fl::lock();					        // lock mutex
            app->output->value(result.c_str());	// perform operation
            Fl::unlock();				        // unlock mutex
            Fl::awake();				        // tell the main thread that smth changed
        } catch (const std::exception& e) {		// if jisho_lookup crashes, display error
            Fl::lock();
            app->output->value(e.what()); 	 	// show the error
            Fl::unlock();
            Fl::awake();
        }
    }).detach();        // let the thread take care of itself afterwards (keep the ui running)
}

void on_search_deepl(Fl_Widget*, void* data){
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();
    if (word.empty()) return;

    std::thread([app, word](){
        try {
            std::string result = deepl_translate(word, api_key);
            Fl::lock();
            app->output->value(result.c_str());
            Fl::unlock();
            Fl::awake();
        } catch (const std::exception& e) {
            Fl::lock();
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
