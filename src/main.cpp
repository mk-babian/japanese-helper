#include <print>

// FLTK
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_PNG_Image.H>

// japanese-helper/src/include
#include "include/colors.h"
#include "include/settings.h"
#include "include/callbacks.h"
#include "include/app_state.h"
#include "include/overrides.h"
#include "include/get_exec_path.h"


// This function handles file path getting for both Linux and
// Windows operating systems
namespace fs = std::filesystem;

fs::path get_executable_path();

int main(void){

#if defined(_WIN32)
    std::println("Compiler says: This is Windows");
#else
    std::println("Compiler says: This is NOT Windows (Linux/Unix)");
#endif

    Fl::scheme("gtk+"); 

    // Hard coded font sizes
    const int small_font = 12;
    const int medium_font = 14;
    const int large_font = 18;

    MainWindow* main_win = new MainWindow(900, 600, "Japanese Helper");
    // Set the platform specific stuff
    #if defined(_WIN32)
        main_win->icon((const void*)LoadIconA(GetModuleHandleA(NULL), "MAINICON"));
        Fl::set_font(FL_FREE_FONT, "Yu Gothic");
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Consolas Bold");
    #elif defined(__APPLE__)
        Fl::set_font(FL_FREE_FONT, "Hiragino Sans");
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Menlo Bold");
    #else
        Fl::set_font(FL_FREE_FONT, "Noto Sans CJK JP");
        Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "DejaVu Sans Mono Bold");
    #endif
    main_win->color(bg_color);

    // Get the path to the executable.
    // Useful for fiding the damn images directory.
    std::string executable_path = get_executable_path().parent_path().string();
    // std::print("{}", executable_path);

    AppState app;

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
    Fl_Choice* choice = new Fl_Choice(10, 10, 120, 30);
    app.api_selector = choice;
    app.api_selector->add("Jisho");
    app.api_selector->add("DeepL");
    app.api_selector->value(0);
    app.api_selector->callback(choice_callback, &app);

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
        std::println("Couldn't load mic_image");
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

    // Create and configure the settings button.
    Fl_Button* settings_btn = new Fl_Button(main_win->w() - 40, 10, 30, 30);
    Fl_PNG_Image* settings_icon = new Fl_PNG_Image((executable_path + "/images/settings.png").c_str());
    if (settings_icon->fail()){
        std::println("Couldn't load settings_image");
    }else{
        settings_btn->image(settings_icon);
    }
    settings_btn->box(FL_UP_BOX);
    settings_btn->color(accent_blue);
    settings_btn->labelcolor(FL_WHITE);
    settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    settings_btn->labelsize(medium_font);
    settings_btn->callback(open_settings, &app);

    // Make the main window resizable.
    main_win->resizable(app.output); 
    main_win->size_range(300, 200);

    // End the main window's parenting spree (I don't know how else to phrase this).
    // Basically, anything after this point is not owned by main_win.
    main_win->end();


    // ============================ SETTINGS WINDOW


    // Create new settings window.
    Fl_Window* settings_win = new Fl_Window(700, 550, "Settings");
    Fl_Box* box = new Fl_Box(4, 40, 692, 4);
    box->box(FL_UP_BOX);
    box->color(bg_color);

    // Create and configure the save button that saves config and closes window.
    Fl_Button* save_button = new Fl_Button(5, 5, 80, 30, "Save");
    save_button->box(FL_UP_BOX);
    save_button->color(accent_blue);
    save_button->labelcolor(FL_WHITE);
    save_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    save_button->callback(on_save_btn, &app);

    // Create and configure the cancel button that closes the window.
    Fl_Button* cancel_button = new Fl_Button(settings_win->w() - 85, 5, 80, 30, "Cancel");
    cancel_button->box(FL_UP_BOX);
    cancel_button->color(accent_red);
    cancel_button->labelcolor(FL_WHITE);
    cancel_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    cancel_button->callback(on_cancel_btn, &app);

    // Create and configure the settings API key input.
    app.settings_key_input = new Fl_Input(90, 50, 200, 30, "DeepL Key:");
    app.settings_key_input->box(FL_UP_BOX);
    app.settings_key_input->color(FL_WHITE);
    app.settings_key_input->textfont(FL_FREE_FONT);
    app.settings_key_input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.settings_key_input->labelcolor(FL_BLACK);

    app.whisper_model_selector = new Fl_Choice(10, 510, 120, 30); 
    app.whisper_model_selector->add("");
    app.whisper_model_selector->add("tiny");
    app.whisper_model_selector->add("base");
    app.whisper_model_selector->add("small");
    app.whisper_model_selector->add("medium");
    app.whisper_model_selector->add("large");
    app.api_selector->value(0);

    settings_win->color(bg_color);
    settings_win->end();

    // Create a keybind to call master_on_search when ENTER is pressed.
    app.input->when(FL_WHEN_ENTER_KEY);
    app.input->callback(master_on_search, &app);

    // Give app settings_win.
    app.settings_win = settings_win;

    // Load the config (the one that the settings window writes).
    // For now, the config is written to the same directory as the executable.
    load_config(&app);

    // Show the main window.
    main_win->show();
 
    Pa_Initialize();                // Start PortAudio.
    Fl::focus(main_win);            // Give focus to the main window.
    Fl::lock();                     // Essential for multithreading.
    int result = Fl::run();         // Start the app.
    Pa_Terminate();                 // Stop PortAudio.
    return result;
}