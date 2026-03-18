#include <FL/Enumerations.H>
#include <iostream>
#include <string>

#include <FL/Fl_Input.H>
#include <Fl/Fl_Window.H>

// override the main window
class MainWindow : public Fl_Window{
private:
    bool is_fullscreen = 0;

public:
    MainWindow(int width, int height, const char* title) : Fl_Window(width, height, title) {}
    
    int handle(int event) override{
        // disable the escape button causing window to exit
        if (event == FL_KEYBOARD && Fl::event_key() == FL_Escape){
            return 1;
        }

        // f11 toggle for fullscreen
        else if (event == FL_KEYBOARD && Fl::event_key() == FL_F + 11 && is_fullscreen == 0){
            Fl_Window::fullscreen();
            is_fullscreen = 1;
            return 1;
        }
        else if (event == FL_KEYBOARD && Fl::event_key() == FL_F + 11 && is_fullscreen == 1){
            Fl_Window::fullscreen_off();
            is_fullscreen = 0;
            return 1;
        }
        return Fl_Window::handle(event);
    }
};

class MainInput : public Fl_Input{
private:
    bool default_text_deleted = false;

public:
    MainInput(int x, int y, int width, int height, const char* label) : Fl_Input(x, y, width, height, label) {}

    int handle(int event) override{
        const std::string text = Fl_Input::value();

        if (!default_text_deleted){
            Fl_Input::textfont(FL_COURIER_ITALIC);
            Fl_Input::textcolor(fl_rgb_color(110, 110, 110));
            if (event == FL_ENTER){
                default_text_deleted = true;
                Fl::set_font(FL_FREE_FONT, "Noto Sans JP");
                Fl_Input::textfont(FL_FREE_FONT); 
                Fl_Input::textcolor(fl_rgb_color(0, 0, 0));
                Fl_Input::value("");
            }
        }

        // clear the input if ESC is pressed & easter egg
        if (event == FL_KEYBOARD && Fl::event_key() == FL_Escape){
            // std::cout << strlen(text) << '\n';
            Fl_Input::value("");
            if (text == "vim"){
                Fl_Input::value("emacs");
            }
            if (text == "emacs"){
                Fl_Input::value("vim");
            }
            return 1;
        } 
        return Fl_Input::handle(event);
    }
};
