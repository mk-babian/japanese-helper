#include <FL/Enumerations.H>
#include <print>
#include <string>
#include <thread>

// FLTK
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Output.H>

// curl
#include <curl/curl.h>
#include <curl/easy.h>

// Parsing
#include "json.hpp"
using json = nlohmann::json;


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out);
std::string jisho_lookup(const std::string& word);
std::string deepl_translate(const std::string& text, const std::string& api_key);

void on_search(Fl_Widget*, void* data);
struct AppState{ // create new struct since only one thingamabob is passable to on_search
    Fl_Input* input;
    Fl_Multiline_Output* output;
};





int main(void){
    Fl::scheme("gtk+"); 
    Fl::background(202, 202, 205); 
    Fl::set_font(FL_FREE_FONT, "Noto Sans JP"); 
    Fl::set_font((Fl_Font)(FL_FREE_FONT + 1), "Google Sans Code Bold");

    Fl_Window* win = new Fl_Window(800, 600, "Japanese Helper");

    AppState app;
   
    // note: i need to remove some of these magic numbers

    app.input = new Fl_Input(200, 10, 400, 30, "Word:");
    app.input->box(FL_RSHADOW_BOX);
    app.input->color(FL_WHITE);
    app.input->textfont(FL_FREE_FONT);
    app.input->textsize(12);
    app.input->textcolor(FL_BLACK);
    app.input->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    app.input->labelsize(14);
    app.input->labelcolor(FL_BLACK);

    app.output = new Fl_Multiline_Output(10, 50, 780, 540);
    app.output->wrap(1);
    app.output->box(FL_RSHADOW_BOX);
    app.output->textfont(FL_FREE_FONT);
    app.output->color(FL_WHITE);
    app.output->textsize(18);

    Fl_Button* btn = new Fl_Button(620, 10, 80, 30, "Search");
    btn->box(FL_RSHADOW_BOX);
    btn->color(fl_rgb_color(0, 120, 215)); 
    btn->labelcolor(FL_WHITE);
    btn->labelfont((Fl_Font)(FL_FREE_FONT + 1));
    btn->labelsize(14);
    btn->callback(on_search, &app);

    win->resizable(app.output); 
    win->size_range(300, 200);
    win->end();
    win->show();
    
    app.input->when(FL_WHEN_ENTER_KEY);
    app.input->callback(on_search, &app);

    Fl::lock();	// enable multithreading i guess
    return Fl::run(); // start the app
}





// networking nonsense below 

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)contents, size * nmemb);
    return size * nmemb;    // check if all the data has been dealt with
}

std::string jisho_lookup(const std::string& word){
    CURL* curl = curl_easy_init();

    std::string response;
    char* encoded = curl_easy_escape(curl, word.c_str(), word.size());  // convert "illegal" characters to hex format
    std::string url = "https://jisho.org/api/v1/search/words?keyword=" + std::string(encoded);
    curl_free(encoded);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");    // tell Jisho that we're Mozilla-man

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());                   // where to make the request
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);                // what headers to attach
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);       // which function handles incoming data chunks
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);               // where that function should write data into
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    // std::println("{}", response);

    auto parsed = json::parse(response);
    auto data = parsed["data"];
    std::string result;
   
    int n = 1;
    // loops to extract all the definitions and etc.
    for (auto& entry : data) {
        for (auto& jp : entry["japanese"]) {
            if (jp.contains("word"))    result += jp["word"].get<std::string>() + " ";              // get the kanji
            if (jp.contains("reading")) result += "(" + jp["reading"].get<std::string>() + ") ";    // get the hiragana
        }
        result += "\n\t";
        n = 1;
        for (auto& sense : entry["senses"]) {
            result += std::to_string(n++) + ". "; 
            for (auto& def : sense["english_definitions"]) {
                result += def.get<std::string>() + ";  ";        // get the english definitions
            }
            result += "\n\t";
        }
        result += "\n\n";
    }

    return result + '\n';
}

std::string deepl_translate(const std::string& text, const std::string& api_key) {
    CURL* curl = curl_easy_init();      // initialize bicep curls
    std::string response;

    std::string postfields = "text=" + text + "&target_lang=EN-US";         // build the message (i.e. text=こにちわ&target_lang=EN-US)
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: DeepL-Auth-Key " + api_key).c_str());

    // configure thingamabobs
    curl_easy_setopt(curl, CURLOPT_URL, "https://api-free.deepl.com/v2/translate");     // where to send
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());                     // what to send
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);                                // credentials
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);                       // use chunk collector (minecraft chunks)
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);                               // collect into this string (response)

    curl_easy_perform(curl);            // send it

    auto parsed = json::parse(response);
    std::string translation = parsed["translations"][0]["text"];
    // println("{}", translation);
    // free memory
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return translation; // raw JSON for now
}

void on_search(Fl_Widget*, void* data){
    auto* app = (AppState*)data;	// cast data to AppState

    std::string word = app->input->value();
    if (word.empty()) return;
    
    // create new thread and give it copies of app and word
    std::thread([app, word](){
        // run at the same time as the main thread (that handles fltk)
        try {
            std::string result = jisho_lookup(word);
            Fl::lock();					// lock mutex
            app->output->value(result.c_str());		// perform operation
            Fl::unlock();				// unlock mutex
            Fl::awake();				// tell the main thread that smth changed
        } catch (const std::exception& e) {		// if jisho_lookup crashes, display error
            Fl::lock();
            app->output->value(e.what()); 		 // show the error
            Fl::unlock();
            Fl::awake();
        }
    }).detach();        // let the thread take care of itself afterwards (keep the ui running)
}
