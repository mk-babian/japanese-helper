#pragma once

#if defined(_WIN32)

#include "app_state.h"

// Registers Ctrl+L as a system-wide hotkey.
// Call AFTER main_win->show(), since it needs a valid HWND.
void register_global_hotkeys(AppState* app);

// Call before app exit to clean up the OS-level registration.
void unregister_global_hotkeys();

#endif