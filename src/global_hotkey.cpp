#include "include/global_hotkey.h"

#if defined(_WIN32)
#include "include/ocr.h"   // capture_windows() — still Windows-only
#endif

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <print>

namespace {

    AppState* g_app = nullptr;

    // FLTK calls this dead last — only once no focused widget, menu
    // shortcut, or window handle() already claimed the event. That's what
    // gives you "fires anywhere in the app" without touching every widget.
    int hotkey_handler(int event){
        if (event != FL_SHORTCUT && event != FL_KEYDOWN){
            return 0;
        }

        if (!(Fl::event_state() & FL_CTRL)){
            return 0;
        }

        switch (Fl::event_key()){
            case 's': {
                std::println("INFO | Ctrl+S pressed");
                int n = g_app->api_selector->size() - 1; // = 3
                int idx = (g_app->api_selector->value() + 1) % n;
                g_app->api_selector->value(idx);
                g_app->api_selector->do_callback();
                return 1;
            }

            default:
                return 0;
        }
    }
}

void register_global_hotkeys(AppState* app){
    g_app = app;
    Fl::add_handler(hotkey_handler);
}

void unregister_global_hotkeys(){
    Fl::remove_handler(hotkey_handler);
}