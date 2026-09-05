// This whole file only compiles on Windows. On Linux/Mac it's skipped entirely,
// because RegisterHotKey and MSG are Windows-only concepts.
#if defined(_WIN32)

#include "include/global_hotkey.h"
#include "include/callbacks.h"
#include "include/ocr.h"

#include <FL/Fl.H>
#include <FL/platform.H>   // gives us fl_xid(), which turns an FLTK window into a native Windows HWND
#include <windows.h>       // the actual Win32 API: RegisterHotKey, MSG, WM_HOTKEY, etc.
#include <print>

// Anonymous namespace = "these things are private to this file only."
// Nothing outside global_hotkey.cpp can see or touch them.
namespace {

    // An ID number we make up ourselves to name this specific hotkey.
    // Windows doesn't know it as "Ctrl+L" internally — it just knows "hotkey #1".
    // If you register more hotkeys later, each one needs its own unique ID.
    constexpr int HOTKEY_CTRL_L_ID = 1;
    // Ctrl+Shift+L triggers the rolling (loopback) transcription. Ctrl+L is taken
    // by OCR, so this needs the extra Shift modifier to stay distinct.
    constexpr int HOTKEY_CTRL_SHIFT_L_ID = 2;

    // A pointer to your app's shared state (the AppState struct from app_state.h).
    // We stash it here because win_message_handler below has no way to receive
    // arguments — FLTK calls it with a fixed signature — so this is the only way
    // to give it access to app.input, app.main_win, etc.
    AppState* g_app = nullptr;

    // This function gets called by FLTK for EVERY raw operating system message,
    // before FLTK itself processes it. Think of it as "peek at the mail before
    // FLTK opens it." Most messages aren't for us, so we just say "not mine" (return 0).
    int win_message_handler(void* event, void*){
        // The 'event' pointer is actually a Windows MSG struct in disguise.
        // We cast it back to its real type so we can read it.
        MSG* msg = static_cast<MSG*>(event);

        // WM_HOTKEY = "a registered hotkey was just pressed, somewhere in the OS."
        // wParam tells us WHICH hotkey (matches the ID we chose above).
        if (msg->message == WM_HOTKEY && msg->wParam == HOTKEY_CTRL_L_ID){
            std::println("INFO | Ctrl+L pressed (global)");

            // Do whatever you want Ctrl+L to trigger here.
            // Right now: just move keyboard focus to the search input box.
            capture_windows();

            return 1; // "I handled this — FLTK, don't process it any further."
        }

        // Ctrl+Shift+L -> transcribe the last 15 seconds of system audio.
        if (msg->message == WM_HOTKEY && msg->wParam == HOTKEY_CTRL_SHIFT_L_ID){
            std::println("INFO | Ctrl+Shift+L pressed (global)");

            // Runs on the main thread (FLTK dispatches system handlers there), so
            // it's safe to snapshot and marshal UI updates the same way the button does.
            on_rolling_btn(nullptr, g_app);

            return 1; // "I handled this — FLTK, don't process it any further."
        }

        return 0; // "Not my message — FLTK, carry on as normal."
    }
}

// Call this ONCE, after your main window is shown, to turn on the global hotkey.
void register_global_hotkeys(AppState* app){
    // Save the app state so win_message_handler can reach it later.
    g_app = app;

    // Convert our FLTK window into the raw Windows handle (HWND) the Win32 API needs.
    // This ONLY works after the window has been shown — before that, it doesn't
    // have a real OS-level handle yet.
    HWND hwnd = fl_xid(app->main_win);

    // Ask Windows: "tell this window whenever Ctrl+L is pressed, anywhere, even if
    // some other app is focused." MOD_CONTROL = the Ctrl modifier, 'L' = the L key.
    if (!RegisterHotKey(hwnd, HOTKEY_CTRL_L_ID, MOD_CONTROL, 'L')){
        // This fails if, e.g., another program already grabbed Ctrl+L. Not fatal —
        // just log it so you know why the hotkey isn't working.
        std::println("W | Failed to register global hotkey Ctrl+L");
    }

    // Register Ctrl+Shift+L for rolling (loopback) transcription.
    if (!RegisterHotKey(hwnd, HOTKEY_CTRL_SHIFT_L_ID, MOD_CONTROL | MOD_SHIFT, 'L')){
        std::println("W | Failed to register global hotkey Ctrl+Shift+L");
    }

    // Plug our handler into FLTK's message pipeline so we actually see the
    // WM_HOTKEY message when it arrives.
    Fl::add_system_handler(win_message_handler, nullptr);
}

// Call this before your app closes, to tell Windows "we're done, release the hotkey."
// Skipping this isn't catastrophic, but it's good hygiene — leaves no dangling
// OS-level registration behind.
void unregister_global_hotkeys(){
    if (g_app && g_app->main_win){
        UnregisterHotKey(fl_xid(g_app->main_win), HOTKEY_CTRL_L_ID);
        UnregisterHotKey(fl_xid(g_app->main_win), HOTKEY_CTRL_SHIFT_L_ID);
    }
}

#endif