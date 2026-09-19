#include <print>
#include <cctype>
#include <cstdlib>
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
#include "include/settings.h"
#include "include/callbacks.h"
#include "include/app_state.h"
#include "include/overrides.h"
#include "include/read_colors.h"
#include "include/get_exec_path.h"
#include "include/global_hotkey.h"
#include "include/history_circ_buffer.h"

// This function handles file path getting for both Linux and
// Windows operating systems
std::filesystem::path get_executable_path();

int main(int argc, char** argv){
#if !defined(_WIN32)
    setenv("FLTK_BACKEND", "x11", 1);
#endif

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
    app.style_colors = read_color_from_file();
    
    std::string api = "";
    std::string term = "";
    for (int i = 1; i < argc; ++i){
        std::string_view arg = argv[i];

        if ((arg == "-a" || arg == "--api") && i + 1 < argc){
            api = argv[++i];
            for (char& c : api){
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }

        if ((arg == "-t" || arg == "--term") && i + 1 < argc){
            term = argv[++i];
            for (char& c : term){
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
    }

    // Create the circular buffer to hold the search history
    CircularBuffer history_circle;
    app.history_buf = &history_circle;
    // Load the config (the one that the settings window writes).
    load_config(&app);
    std::println("INFO | History capacity: {}", app.history_capacity);
    std::println("INFO | History buf capacity: {}", app.history_buf->capacity);
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
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 2), "DejaVu Sans Mono Oblique");
    #endif
    main_win->color(app.style_colors.bg_color);

    // Create and configure the main "search" input box.
    app.input = new MainInput(250, 10, 400, 30, "");
    app.input->box(FL_UP_BOX);
    app.input->value("Input text here...");
    app.input->textfont(FL_FREE_FONT);
    app.input->textsize(small_font);
    app.input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.input->labelsize(medium_font);
    set_widget_fill(app.input, app.style_colors.bg_color);
    const Fl_Color input_text_color = readable_label_color(app.style_colors.bg_color);
    app.input->set_text_color(input_text_color);
    app.input->textcolor(input_text_color);

    // Create and configure the output box.
    // The output box is an OverlayOutput; see `overrides.h`
    OverlayOutput* output = new OverlayOutput(10, 50, 880, 540);
    app.output = output;
    app.output->wrap(1);
    app.output->textfont(FL_FREE_FONT);
    app.output->textsize(large_font);
    set_widget_fill(app.output, app.style_colors.bg_color);
    app.output->textcolor(readable_label_color(app.style_colors.bg_color));

    // Hide the output until the user performs a search.
    app.output->hide();

    // Show a small hint in its place.
    app.search_hint = new Fl_Box(10, 50, 880, 540, "Search to get started.");
    app.search_hint->box(FL_FLAT_BOX);
    set_widget_fill(app.search_hint, app.style_colors.bg_color);
    app.search_hint->labelfont((Fl_Font)(FL_FREE_FONT + 2));
    app.search_hint->labelsize(medium_font);
    app.search_hint->align(FL_ALIGN_CENTER);

    app.anki_button = new Fl_Button(850, 550, 30, 30, "A");
    app.anki_button->box(FL_UP_BOX);
    app.anki_button->callback(on_anki_button, &app);
    app.anki_button->hide();
    output->keep_on_top(app.anki_button);

    // Create and configure a chocie for the selected API
    Fl_Choice* choice = new Fl_Choice(45, 10, 120, 30);
    app.api_selector = choice;
    app.api_selector->add("Jisho");
    app.api_selector->add("DeepL");
    app.api_selector->add("MyMemory");
    app.api_selector->value(app.selected_api);
    app.api_selector->callback(choice_callback, &app);
    set_widget_fill(app.api_selector, app.style_colors.bg_color);
    app.api_selector->textcolor(readable_label_color(app.style_colors.bg_color));

    Fl_Button* info_button = new Fl_Button(10, 10, 30, 30);
    info_button->box(FL_UP_BOX);
    info_button->color(app.style_colors.accent_2);
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
    app.stt_btn->color(app.style_colors.accent_2);
    app.stt_btn->selection_color(app.style_colors.accent_2);          // Prevents button turning grey when clicked.
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
    set_widget_fill(app.search_btn, app.style_colors.accent_2);
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
    set_widget_fill(history_btn, app.style_colors.bg_color);

    // Create and configure the settings button.
    Fl_Button* settings_btn = new Fl_Button(main_win->w() - 40, 10, 30, 30);
    Fl_PNG_Image* settings_icon = new Fl_PNG_Image((executable_path + "/images/settings.png").c_str());
    if (settings_icon->fail()){
        std::println("W | Couldn't load settings-icon!");
    }else{
        settings_btn->image(settings_icon);
    }
    settings_btn->box(FL_UP_BOX);
    set_widget_fill(settings_btn, app.style_colors.accent_2);
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

    if (!api.empty()){
        if (api == "jisho"){
            app.api_selector->value(0);
            app.api_selector->do_callback();
        }else if(api == "deepl"){
            app.api_selector->value(1);
            app.api_selector->do_callback();
        }else if(api == "mymemory"){
            app.api_selector->value(2);
            app.api_selector->do_callback();
        }
    }

    if (!term.empty()){
        app.input->value(term.c_str());
        app.search_btn->do_callback();
    }
    
    // ============================ SETTINGS WINDOW


    // Create new settings window.
    app.settings_win = new Fl_Window(700, 550, "Settings");
    app.settings_win->color(app.style_colors.bg_color);
    // Fl_Box* box = new Fl_Box(4, 40, 692, 4);
    // box->box(FL_UP_BOX);
    // box->color(app.style_colors.bg_color);

    app.settings_content = new Fl_Group(180, 0, 520, 550);
    app.settings_content->box(FL_FLAT_BOX);
    app.settings_content->color(app.style_colors.bg_color);
    app.settings_content->end();

    Fl_Box* left_box = new Fl_Box(0, 0, 180, 550);
    left_box->box(FL_FLAT_BOX);
    left_box->color(app.style_colors.bg_color);

    Fl_Button* general_btn = new Fl_Button(10, 10, 160, 30, "General");
    general_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    general_btn->box(FL_UP_BOX);
    set_widget_fill(general_btn, app.style_colors.accent_2);
    general_btn->callback(on_settings_win_change, &app);
    app.general_settings_btn = general_btn;

    Fl_Button* history_settings_btn = new Fl_Button(10, 45, 160, 30, "History");
    history_settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    history_settings_btn->box(FL_UP_BOX);
    set_widget_fill(history_settings_btn, app.style_colors.accent_2);
    history_settings_btn->callback(on_settings_win_change, &app);
    app.history_settings_btn = history_settings_btn;

    Fl_Button* api_settings_btn = new Fl_Button(10, 80, 160, 30, "API");
    api_settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    api_settings_btn->box(FL_UP_BOX);
    set_widget_fill(api_settings_btn, app.style_colors.accent_2);
    api_settings_btn->callback(on_settings_win_change, &app);
    app.api_settings_btn = api_settings_btn;

    Fl_Button* stt_settings_btn = new Fl_Button(10, 115, 160, 30, "Speech-to-Text");
    stt_settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    stt_settings_btn->box(FL_UP_BOX);
    set_widget_fill(stt_settings_btn, app.style_colors.accent_2);
    stt_settings_btn->callback(on_settings_win_change, &app);
    app.stt_settings_btn = stt_settings_btn;

    // Create and configure the save button that saves config and closes window.
    Fl_Button* save_button = new Fl_Button(7.5, app.settings_win->h() - 35, 80, 30, "Apply");
    save_button->box(FL_UP_BOX);
    set_widget_fill(save_button, app.style_colors.accent_2);
    save_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    save_button->callback(on_apply_btn, &app);

    // Create and configure the cancel button that closes the window.
    Fl_Button* cancel_button = new Fl_Button(92.5, app.settings_win->h() - 35, 80, 30, "Cancel");
    cancel_button->box(FL_UP_BOX);
    set_widget_fill(cancel_button, app.style_colors.accent_0);
    cancel_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    cancel_button->callback(on_cancel_btn, &app);

    Fl_Box* separator = new Fl_Box(179, 0, 2, 550);
    separator->color(app.style_colors.fg_color);
    separator->align(FL_ALIGN_CENTER);
    separator->box(FL_FLAT_BOX);

    app.settings_win->end();


    // ============================ HISTORY WINDOW


    app.history_win = new Fl_Window(310, 500, "History");
    Fl_Scroll* scroll = new Fl_Scroll(0, 0, 310, 500);
    app.history_scroll = scroll;
    app.history_scroll->type(Fl_Scroll::VERTICAL_ALWAYS);
    app.history_scroll->color(app.style_colors.bg_color);
    app.history_scroll->scrollbar.color(app.style_colors.bg_color);
    app.history_scroll->scrollbar.selection_color(app.style_colors.accent_2);
    app.history_scroll->end();
    app.history_win->color(app.style_colors.bg_color);
    app.history_win->end();

    // Create a keybind to call master_on_search when ENTER is pressed.
    app.input->when(FL_WHEN_ENTER_KEY);
    app.input->callback(master_on_search, &app);

    // Show the main window.
    main_win->show();

    register_global_hotkeys(&app);

    
    // ============================ INFO WINDOW

    
    app.info_win = new Fl_Window(500, 600, "Info");
    set_widget_fill(app.info_win, app.style_colors.bg_color);

    app.info_content = new Fl_Group(200, 0, 300, 600);
    app.info_content->box(FL_FLAT_BOX);
    set_widget_fill(app.info_content, app.style_colors.bg_color);
    app.info_content->end();

    Fl_Box* info_left_box = new Fl_Box(0, 0, 200, 600);
    info_left_box->box(FL_FLAT_BOX);
    info_left_box->color(app.style_colors.bg_color);
    
    Fl_Button* general_info_btn = new Fl_Button(10, 5, 180, 30, "General");
    general_info_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    general_info_btn->box(FL_UP_BOX);
    set_widget_fill(general_info_btn, app.style_colors.accent_2);
    general_info_btn->callback(on_info_win_change, &app);
    app.general_info_btn = general_info_btn;

    Fl_Button* api_info_btn = new Fl_Button(10, 40, 180, 30, "API");
    api_info_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    api_info_btn->box(FL_UP_BOX);
    set_widget_fill(api_info_btn, app.style_colors.accent_2);
    api_info_btn->callback(on_info_win_change, &app);
    app.api_info_btn = api_info_btn;

    Fl_Button* whisper_info_btn = new Fl_Button(10, 75, 180, 30, "Whisper");
    whisper_info_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    whisper_info_btn->box(FL_UP_BOX);
    set_widget_fill(whisper_info_btn, app.style_colors.accent_2);
    whisper_info_btn->callback(on_info_win_change, &app);
    app.whisper_info_btn = whisper_info_btn;

    app.info_win->end();

    curl_global_init(CURL_GLOBAL_ALL);  // Must be called before any threads use curl.
    Pa_Initialize();                    // Start PortAudio.
    Fl::focus(main_win);                // Give focus to the main window.
    app.input->take_focus();            // Move keyboard focus to the search input box.
    Fl::lock();                         // Essential for multithreading.
    int result = Fl::run();             // Start the app.
    Pa_Terminate();                     // Stop PortAudio.
    curl_global_cleanup();
    unregister_global_hotkeys();
    return result;
}
