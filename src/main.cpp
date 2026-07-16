#include <print>
#include <curl/curl.h>

// FLTK
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Tooltip.H>

// japanese-helper/src/include
#include "include/colors.h"
#include "include/settings.h"
#include "include/callbacks.h"
#include "include/app_state.h"
#include "include/overrides.h"
#include "include/get_exec_path.h"
#include "include/history_circ_buffer.h"

// This function handles file path getting for both Linux and
// Windows operating systems
std::filesystem::path get_executable_path();

int main(void){
#if defined(_WIN32)
    std::println("INFO | Compiler says: This is Windows");
#else
    std::println("INFO | Compiler says: This is NOT Windows (Linux/Unix)");
#endif

    Fl::scheme("gtk+"); 
    Fl_Tooltip::color(FL_WHITE); 
    Fl_Tooltip::delay(0.25f);

    // Font sizes
    const int small_font = 12;
    const int medium_font = 14;
    const int large_font = 18;

    // Get the path to the executable.
    // Useful for fiding the damn images directory.
    std::string executable_path = get_executable_path().parent_path().string();
    // std::print("{}", executable_path);

    AppState app;

    // Create the circular buffer to hold the search history
    CircularBuffer history_circle;
    app.history_buf = &history_circle;
    // Load the config (the one that the settings window writes).
    load_config(&app);
    // Allocate memory for strings
    app.history_buf->data.resize(app.history_buf->capacity);
    app.history_buf->time.resize(app.history_buf->capacity);
    app.history_buf->api.resize(app.history_buf->capacity);
    app.history_buf->head = 0;
    app.history_buf->tail = 0;
    app.history_buf->size = 0;

    load_buffer(&app);
    print_buffers(&app);

    /* 
     * The code below creates all of the necessary FLTK widgets.
     *
     * Most of them are passed to AppState, a struct defined in include\app_state.h
     * It's used as a way for callback functions to comunicate with the FLTK widgets.
     *
     * Without it, most of these operations could not be completed, since the callback
     * functions can only accept one other variable as an argument, therefore a struct is necessary.
    */


    // ============================ MAIN WINDOW

    MainWindow* main_win = new MainWindow(900, 600, "Japanese Helper");
    app.main_win = main_win;

    // Set the platform specific stuff
    #if defined(_WIN32)
        main_win->icon((const void*)LoadIconA(GetModuleHandleA(NULL), "MAINICON"));
        Fl::set_font(FL_FREE_FONT, "Yu Gothic");
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Consolas Bold");
    #elif defined(__APPLE__)
        Fl::set_font(FL_FREE_FONT, "Hiragino Sans");
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Menlo Bold");
    #else
        Fl_PNG_Image icon("../app_icon.png");
        main_win->icon(&icon);
        Fl::set_font(FL_FREE_FONT, "Noto Sans CJK JP");
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "DejaVu Sans Mono Bold");
    #endif
    main_win->color(bg_color);

    // Create and configure the main "search" input box.
    app.input = new MainInput(250, 10, 400, 30, "");
    app.input->box(FL_UP_BOX);
    app.input->color(FL_WHITE);
    app.input->value("Input text here...");
    app.input->textfont(FL_FREE_FONT);
    app.input->textsize(small_font);
    app.input->textcolor(FL_BLACK);
    app.input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.input->labelsize(medium_font);
    app.input->labelcolor(FL_BLACK);

    // Create and configure the output box.
    app.output = new Fl_Multiline_Output(10, 50, 880, 540);
    app.output->wrap(1);
    app.output->box(FL_UP_BOX);
    app.output->textfont(FL_FREE_FONT);
    app.output->color(FL_WHITE);
    app.output->textsize(large_font);

    // Create and configure a chocie for the selected API
    Fl_Choice* choice = new Fl_Choice(45, 10, 120, 30);
    app.api_selector = choice;
    app.api_selector->add("Jisho");
    app.api_selector->add("DeepL");
    app.api_selector->add("MyMemory");
    app.api_selector->value(0);
    app.api_selector->callback(choice_callback, &app);

    Fl_Button* info_button = new Fl_Button(10, 10, 30, 30);
    info_button->box(FL_UP_BOX);
    info_button->color(accent_blue);
    Fl_PNG_Image* info_icon = new Fl_PNG_Image((executable_path + "/images/info.png").c_str());
    if (info_icon->fail()){
        std::println("W | Couldn't load info-icon image!");
    }else{
        info_button->image(info_icon);
    }
    info_button->callback(open_info, &app);

    // Create and configure the microphone button for STT
    Fl_Button* voice_to_text_btn = new Fl_Button(215, 10, 30, 30);
    app.stt_btn = voice_to_text_btn;
    app.stt_btn->color(accent_blue);
    app.stt_btn->selection_color(accent_blue);          // Prevents button turning grey when clicked.
    app.stt_btn->clear_visible_focus();                 // Prevents the GTK+ scheme from grey-boxing it.
    app.stt_btn->box(FL_UP_BOX);
    app.stt_btn->callback(on_stt_btn, &app);
    // Create a new Fl_PNG_Image.
    Fl_PNG_Image* mic_icon = new Fl_PNG_Image((executable_path + "/images/microphone.png").c_str());
    // Check for errors loading the image file.
    if (mic_icon->fail()){
        std::println("W | Couldn't load mic-icon image!");
    }else{
        app.stt_btn->image(mic_icon);
    }

    // Create and configure the main search button.
    app.search_btn = new Fl_Button(655, 10, 80, 30, "Search");
    app.search_btn->box(FL_UP_BOX);
    app.search_btn->color(accent_blue); 
    app.search_btn->labelcolor(FL_WHITE);
    app.search_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.search_btn->labelsize(medium_font);
    app.search_btn->callback(master_on_search, &app);

    Fl_Button* history_btn = new Fl_Button(main_win->w() - 75, 10, 30, 30);
    Fl_PNG_Image* history_icon = new Fl_PNG_Image((executable_path + "/images/history-icon.png").c_str());
    if (history_icon->fail()){
        std::println("W | Couldn't load history-icon!");
    }else{
        history_btn->image(history_icon);
    }
    history_btn->box(FL_UP_BOX);
    history_btn->callback(on_history_btn, &app);

    // Create and configure the settings button.
    Fl_Button* settings_btn = new Fl_Button(main_win->w() - 40, 10, 30, 30);
    Fl_PNG_Image* settings_icon = new Fl_PNG_Image((executable_path + "/images/settings.png").c_str());
    if (settings_icon->fail()){
        std::println("W | Couldn't load settings-icon!");
    }else{
        settings_btn->image(settings_icon);
    }
    settings_btn->box(FL_UP_BOX);
    settings_btn->color(accent_blue);
    settings_btn->labelcolor(FL_WHITE);
    settings_btn->callback(open_settings, &app);

    // Make the main window resizable.
    main_win->resizable(app.output); 
    main_win->size_range(300, 200);

    // End the main window's parenting spree (I don't know how else to phrase this).
    // Basically, anything after this point is not owned by main_win.
    main_win->end();
    // Give the main window a callback when hiding it
    // The callback writes to the search history file before closing
    main_win->callback(on_main_win_close, &app);

    
    // ============================ SETTINGS WINDOW


    // Create new settings window.
    app.settings_win = new Fl_Window(700, 550, "Settings");
    // Fl_Box* box = new Fl_Box(4, 40, 692, 4);
    // box->box(FL_UP_BOX);
    // box->color(bg_color);

    app.settings_content = new Fl_Group(180, 0, 520, 550);
    app.settings_content->box(FL_FLAT_BOX);
    app.settings_content->color(bg_color);
    app.settings_content->end();

    Fl_Box* left_box = new Fl_Box(0, 0, 180, 550);
    left_box->box(FL_FLAT_BOX);
    left_box->color(fl_rgb_color(180, 180, 185));

    Fl_Button* general_btn = new Fl_Button(10, 10, 160, 30, "General");
    general_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    general_btn->box(FL_UP_BOX);
    general_btn->color(accent_blue);
    general_btn->labelcolor(FL_WHITE);
    general_btn->callback(on_settings_win_change, &app);
    app.general_settings_btn = general_btn;

    Fl_Button* history_settings_btn = new Fl_Button(10, 45, 160, 30, "History");
    history_settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    history_settings_btn->box(FL_UP_BOX);
    history_settings_btn->color(accent_blue);
    history_settings_btn->labelcolor(FL_WHITE);
    history_settings_btn->callback(on_settings_win_change, &app);
    app.history_settings_btn = history_settings_btn;

    Fl_Button* api_settings_btn = new Fl_Button(10, 80, 160, 30, "API");
    api_settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    api_settings_btn->box(FL_UP_BOX);
    api_settings_btn->color(accent_blue);
    api_settings_btn->labelcolor(FL_WHITE);
    api_settings_btn->callback(on_settings_win_change, &app);
    app.api_settings_btn = api_settings_btn;

    Fl_Button* stt_settings_btn = new Fl_Button(10, 115, 160, 30, "Speech-to-Text");
    stt_settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    stt_settings_btn->box(FL_UP_BOX);
    stt_settings_btn->color(accent_blue);
    stt_settings_btn->labelcolor(FL_WHITE);
    stt_settings_btn->callback(on_settings_win_change, &app);
    app.stt_settings_btn = stt_settings_btn;

    // Create and configure the save button that saves config and closes window.
    Fl_Button* save_button = new Fl_Button(5, app.settings_win->h() - 35, 80, 30, "Apply");
    save_button->box(FL_UP_BOX);
    save_button->color(accent_blue);
    save_button->labelcolor(FL_WHITE);
    save_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    save_button->callback(on_apply_btn, &app);

    // Create and configure the cancel button that closes the window.
    Fl_Button* cancel_button = new Fl_Button(95, app.settings_win->h() - 35, 80, 30, "Cancel");
    cancel_button->box(FL_UP_BOX);
    cancel_button->color(accent_red);
    cancel_button->labelcolor(FL_WHITE);
    cancel_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    cancel_button->callback(on_cancel_btn, &app);

    app.settings_win->color(bg_color);
    app.settings_win->end();


    // ============================ HISTORY WINDOW


    app.history_win = new Fl_Window(310, 500, "History");
    Fl_Scroll* scroll = new Fl_Scroll(0, 0, 310, 500);
    app.history_scroll = scroll;
    app.history_scroll->type(Fl_Scroll::VERTICAL_ALWAYS);
    app.history_scroll->end();
    app.history_win->end();

    // Create a keybind to call master_on_search when ENTER is pressed.
    app.input->when(FL_WHEN_ENTER_KEY);
    app.input->callback(master_on_search, &app);

    // Show the main window.
    main_win->show();

    
    // ============================ INFO WINDOW

    
    app.info_win = new Fl_Window(500, 600, "Info");

    app.info_content = new Fl_Group(200, 0, 300, 600);
    app.info_content->box(FL_FLAT_BOX);
    app.info_content->color(bg_color);
    app.info_content->end();

    Fl_Box* info_left_box = new Fl_Box(0, 0, 200, 600);
    info_left_box->box(FL_FLAT_BOX);
    info_left_box->color(fl_rgb_color(180, 180, 185));

    Fl_Button* general_info_btn = new Fl_Button(10, 10, 180, 30, "General");
    general_info_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    general_info_btn->box(FL_UP_BOX);
    general_info_btn->color(accent_blue);
    general_info_btn->labelcolor(FL_WHITE);
    general_info_btn->callback(on_info_win_change, &app);
    app.general_info_btn = general_info_btn;

    Fl_Button* api_info_btn = new Fl_Button(10, 50, 180, 30, "API");
    api_info_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    api_info_btn->box(FL_UP_BOX);
    api_info_btn->color(accent_blue);
    api_info_btn->labelcolor(FL_WHITE);
    api_info_btn->callback(on_info_win_change, &app);
    app.api_info_btn = api_info_btn;

    Fl_Button* whisper_info_btn = new Fl_Button(10, 90, 180, 30, "Whisper");
    whisper_info_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    whisper_info_btn->box(FL_UP_BOX);
    whisper_info_btn->color(accent_blue);
    whisper_info_btn->labelcolor(FL_WHITE);
    whisper_info_btn->callback(on_info_win_change, &app);
    app.whisper_info_btn = whisper_info_btn;

    app.info_win->color(bg_color);
    app.info_win->end();


    curl_global_init(CURL_GLOBAL_ALL);  // Must be called before any threads use curl.
    Pa_Initialize();                    // Start PortAudio.
    Fl::focus(main_win);                // Give focus to the main window.
    Fl::lock();                         // Essential for multithreading.
    int result = Fl::run();             // Start the app.
    Pa_Terminate();                     // Stop PortAudio.
    curl_global_cleanup();
    return result;
}