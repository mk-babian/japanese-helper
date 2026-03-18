#include <thread>

#include <Fl/Fl_Choice.H>

#include "include/callbacks.h"
#include "include/api.h"
#include "include/app_state.h"
#include "include/settings.h"

const Fl_Color bg_color = fl_rgb_color(202, 202, 205);
const Fl_Color accent_blue = fl_rgb_color(0, 120, 215);

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

void on_search_jisho(Fl_Widget*, void* data){
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();

    // create new thread and give it copies of app and word
    std::thread([app, word](){
        // run at the same time as the main thread (that handles fltk)
        try {
            std::string result = jisho_lookup(word);
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

void on_search_deepl(Fl_Widget*, void* data){
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();

    std::thread([app, word](){
        try {
            std::string result = deepl_translate(word, app->deepl_key);
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
    AppState* app = static_cast<AppState*>(data);

    app->settings_win->show();
}

void on_save_btn(Fl_Widget *w, void *data){
    AppState* app = static_cast<AppState*>(data); 

    std::string key = app->settings_key_input->value(); 
    if (key.empty()) return;
    app->deepl_key = key;

    save_config(app);

    app->settings_win->hide();
}
