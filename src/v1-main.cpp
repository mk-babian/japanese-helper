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
struct AppState{
    Fl_Input* input;
    Fl_Multiline_Output* output;
};





int main(void){
    // std::string result = translate("あか、あお", "REDACTED");
    // std::println("{}", result);
    // std::println("\n{}", jisho_lookup("あか"));
   
    // i honestly have no clue how to use FLTK
    Fl_Window* win = new Fl_Window(400, 300, "Japanese Helper");

    AppState app;
    app.input = new Fl_Input(80, 10, 220, 30, "Word:");
    app.output = new Fl_Multiline_Output(10, 50, 380, 240);
    app.output->wrap(1);

    Fl_Button* btn = new Fl_Button(310, 10, 80, 30, "Search");
    btn->callback(on_search, &app);

    win->end();
    win->show();
    
    Fl::lock();
    return Fl::run();
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
        result += std::to_string(n++) + ". "; 
        for (auto& jp : entry["japanese"]) {
            if (jp.contains("word"))    result += jp["word"].get<std::string>() + " ";              // get the kanji
            if (jp.contains("reading")) result += "(" + jp["reading"].get<std::string>() + ") ";    // get the hiragana
        }
        for (auto& sense : entry["senses"]) {
            for (auto& def : sense["english_definitions"]) {
                result += def.get<std::string>() + ", ";        // get the english definitions
            }
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
    auto* app = (AppState*)data;

    std::string word = app->input->value();
    if (word.empty()) return;
    
    std::thread([app, word](){
        std::string result = jisho_lookup(word);
        Fl::lock();
        app->output->value(result.c_str());
        Fl::unlock();
        Fl::awake();
    }).detach();
}
