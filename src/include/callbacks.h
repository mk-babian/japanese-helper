#include <FL/Fl_Widget.H>

#include "app_state.h"

// Helper struct to pass multiple pieces of data to the history entry callback
typedef struct HistoryEntryData {
    AppState* app;
    std::string query;
    int api;
} HistoryEntryData;

void on_search_jisho(Fl_Widget* w, void* data);
void on_search_deepl(Fl_Widget* w, void* data);
void on_search_mymemory(Fl_Widget* w, void* data);
void choice_callback(Fl_Widget* w, void* data);
void master_on_search(Fl_Widget* w, void* data);
void open_settings(Fl_Widget* w, void* data);
void on_apply_btn(Fl_Widget* w, void* data);
void on_cancel_btn(Fl_Widget* w, void* data);
void on_stt_btn(Fl_Widget* w, void* data);
void on_history_btn(Fl_Widget* w, void* data);
void on_main_win_close(Fl_Widget* w, void* data);
void on_settings_win_change(Fl_Widget* w, void* data);
void on_clear_history_btn(Fl_Widget* w, void* data);
void on_history_entry_click(Fl_Widget* w, void* user_data);
void model_choice_callback(Fl_Widget* w, void* data);
void download_button(Fl_Widget* w, void* data);
void show_deepl_key_btn(Fl_Widget* w, void* data);