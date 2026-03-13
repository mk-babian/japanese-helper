#include <print>

// FLTK
#include <FL/Fl.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Multiline_Output.H>

#include "include/api.h"
#include "include/api_key.h"
#include "include/callbacks.h"
#include "include/app_state.h"

int main(void){
    int small_font = 12;
    int medium_font = 14;
    int large_font = 18;

    int margin = 10;

    Fl_Color bg_color = fl_rgb_color(202, 202, 205);
    Fl_Color accent_blue = fl_rgb_color(0, 120, 215);

    Fl::scheme("gtk+"); 
    Fl::set_font(FL_FREE_FONT, "Noto Sans JP"); 
    Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Google Sans Code Bold");

    Fl_Window* win = new Fl_Window(800, 600, "Japanese Helper");
    win->color(bg_color);

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

    Fl_Button* btn = new Fl_Button(620, margin, 80, 30, "Search");
    btn->box(FL_RSHADOW_BOX);
    btn->color(accent_blue); 
    btn->labelcolor(FL_WHITE);
    btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    btn->labelsize(medium_font);
    btn->callback(master_on_search, &app);

    win->resizable(app.output); 
    win->size_range(300, 200);
    win->end();
    win->show();
    
    app.input->when(FL_WHEN_ENTER_KEY);
    app.input->callback(master_on_search, &app);

    Fl::lock();	// enable multithreading i guess
    return Fl::run(); // start the app
}
