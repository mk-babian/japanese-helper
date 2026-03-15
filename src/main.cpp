#include <print>
#include <windows.h>

// FLTK
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Multiline_Output.H>

#include "include/api.h"
#include "include/settings.h"
#include "include/callbacks.h"
#include "include/app_state.h"

int main(void){
    Fl::scheme("gtk+"); 

    int small_font = 12;
    int medium_font = 14;
    int large_font = 18;

    int margin = 10;

    Fl_Color bg_color = fl_rgb_color(202, 202, 205);
    Fl_Color accent_blue = fl_rgb_color(0, 120, 215);

    Fl::set_font(FL_FREE_FONT, "Noto Sans JP"); 
    Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Google Sans Code Bold");

    Fl_Window* main_win = new Fl_Window(800, 600, "Japanese Helper");
    main_win->color(bg_color);
    main_win->icon((const void*)LoadIconA(GetModuleHandleA(NULL), "MAINICON"));

    AppState app;
   
    // note: i need to remove some of these magic numbers

    app.input = new Fl_Input(200, margin, 400, 30, "Word:");
    app.input->box(FL_RSHADOW_BOX);
    app.input->color(FL_WHITE);
    app.input->textfont(FL_FREE_FONT);
    app.input->textsize(small_font);
    app.input->textcolor(FL_BLACK);
    app.input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.input->labelsize(medium_font);
    app.input->labelcolor(FL_BLACK);

    app.output = new Fl_Multiline_Output(margin, 50, 780, 540);
    app.output->wrap(1);
    app.output->box(FL_RSHADOW_BOX);
    app.output->textfont(FL_FREE_FONT);
    app.output->color(FL_WHITE);
    app.output->textsize(large_font);

    Fl_Choice* choice = new Fl_Choice(margin, margin, 120, 30);
    choice->add("Jisho");
    choice->add("DeepL");
    choice->value(0);
    choice->callback(choice_callback, &app);

    Fl_Button* search_btn = new Fl_Button(620, margin, 80, 30, "Search");
    app.search_btn = search_btn;
    search_btn->box(FL_RSHADOW_BOX);
    search_btn->color(accent_blue); 
    search_btn->labelcolor(FL_WHITE);
    search_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    search_btn->labelsize(medium_font);
    search_btn->callback(master_on_search, &app);

    Fl_Button* settings_btn = new Fl_Button(710, margin, 80, 30, "Settings");
    settings_btn->box(FL_RSHADOW_BOX);
    settings_btn->color(accent_blue);
    settings_btn->labelcolor(FL_WHITE);
    settings_btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    settings_btn->labelsize(medium_font);
    settings_btn->callback(open_settings, &app);

    main_win->resizable(app.output); 
    main_win->size_range(300, 200);
    main_win->end();

    Fl_Window* settings_win = new Fl_Window(700, 550, "Settings");
    Fl_Box* box = new Fl_Box(4, 40, 692, 4);
    box->box(FL_RSHADOW_BOX);
    box->color(bg_color);
    Fl_Button* save_button = new Fl_Button(5, 5, 80, 30, "Save");
    save_button->box(FL_RSHADOW_BOX);
    save_button->color(accent_blue);
    save_button->labelcolor(FL_WHITE);
    save_button->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    save_button->callback(on_save_btn, &app);
    app.settings_key_input = new Fl_Input(90, 50, 200, 30, "DeepL Key:");
    app.settings_key_input->box(FL_RSHADOW_BOX);
    app.settings_key_input->color(FL_WHITE);
    app.settings_key_input->textfont(FL_FREE_FONT);
    app.settings_key_input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.settings_key_input->labelcolor(FL_BLACK);
    settings_win->color(bg_color);
    settings_win->end();

    app.input->when(FL_WHEN_ENTER_KEY);
    app.input->callback(master_on_search, &app);
    app.settings_win = settings_win;

    load_config(&app);

    main_win->show();

    Fl::lock();	// enable multithreading i guess
    return Fl::run(); // start the app
}
