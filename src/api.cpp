// For HTTP codes
#include <map>

// CURL
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/urlapi.h>

// JSON
#include "lib/json.hpp"
using json = nlohmann::json;

// Anki Connect
#include <print>

#include "include/api.h"
#include "include/app_state.h"



// COUNTS HOW MANY BYTES HANDLED IN CHUNK & COMPARE WITH RECEIVED AMOUNT
static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)contents, size * nmemb); // Append to contents.
    return size * nmemb;    // Check if all the data has been dealt with.
                            // If handled bytes don't match with received bytes,
                            // then libcurl aborts with CURLE_WRITE_ERROR.
}

/* RESOURCE ACQUISISTION IS INITIALIZATION (RAII)
 *
 * RAII lets an object own the resource,
 * and let it clean itself up with its destructor.
 *
 * Destructors run automatically when the object
 * goes out of scope (return, exit, throw, etc.).
*/
struct Cleanup{
    CURL* curl;
    struct curl_slist* headers;

    Cleanup(CURL* c, struct curl_slist* h){
        curl = c;
        headers = h;
    }

    ~Cleanup(){
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
};

// Looks up a word on Jisho and returns a formatted string of definitions, Kanji, and Hiragana (reading).
// Throws std::runtime_error on network failure or malformed response.
std::string jisho_lookup(const std::string& word, AppState* app){
    CURL* curl = curl_easy_init();                                      // Initialize bicep curls.
                                                                        // Allocate and return a CURL* handle.
    std::string response;
    std::string result;

    // Encode the chracters since URLs only allow a limited set of characters.
    // Everything else must be percent encoded (like Japanese characters, だってばよ).
    char* encoded = curl_easy_escape(curl, word.c_str(), word.size());
    std::string url = "https://jisho.org/api/v1/search/words?keyword=" + std::string(encoded);
    curl_free(encoded);

    // Give User-Agent headers since Jisho gives 403 Forbidden;
    // It blocks requests with no User-Agent.
    struct curl_slist* headers = nullptr; 
    headers = curl_slist_append(headers, 
            "User-Agent: Mozilla/5.0 (Windows 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    
    // Create object, own the curl + headers;
    Cleanup cl(curl, headers);

    // Use c_str since libcurl is a C library, therefore it needs C-like strings.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());                   // Where to make the request.
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);                // What headers to attach.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);      // Which function handles incoming data chunks.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);               // Where that function should write data into.
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK){                                               // Check for errors.
        throw std::runtime_error(curl_easy_strerror(res));
    }

    // Check if the response is emtpy using two methods.
    // A response is empty when it's not a JSON, a garbage HTML,
    // an error page, or entirely empty.
    if (response.empty() || response[0] != '{'){
        // Thorws an error at runtime if the above if statement is met.
        // The exception travels up the call stack to the main caller.
        // The catcher can call .what() on this exception to get the message back.
        throw std::runtime_error("W | Jisho returned non-JSON response:\n" + response.substr(0, 200));
    }
    
    app->raw_jisho_return = response;

    // Parse the raw bytes/characters into std::string result.
    auto parsed = json::parse(response);
    auto data = parsed["data"];
    // Check if the data is empty, and Jisho could not actually
    // find any relative inforamtion.
    if (data.empty()){
        result = "No results found.";
        return result;
    }

    // Integer counter for the formatting
    int n = 1;

    // Loops to extract all the definitions and etc. from the parsed data
    // and puts them into std::string result.
    /*
     * The ":" means "for each element in".
     * auto means to "figure out the type yourself, compiler-san".
     * & means "reference, not copy" - without it, entry would be a copy of each JSON element.
    */
    for (auto& entry : data) {
        for (auto& jp : entry["japanese"]) {
            if (jp.contains("word"))    result += jp["word"].get<std::string>() + " ";              // Get the Kanji
            if (jp.contains("reading")) result += "(" + jp["reading"].get<std::string>() + ") ";    // Get the Hiragana
        }
        result += "\n\t";
        // Reset the visual counter after each new form of the word.
        n = 1;
        for (auto& sense : entry["senses"]) {
            result += std::to_string(n++) + ". "; 
            for (auto& def : sense["english_definitions"]) {
                result += def.get<std::string>() + ";  ";        // Get the english definitions
            }
            result += "\n\t";
        }
        result += "\n\n";
    }

    std::println("INFO | Jisho Result: {}", result + '\n');

    return result + '\n';
}

// IN: std::string text and the DeepL API key
// OUT: parsed std::string translation
std::string deepl_translate(const std::string& text, const std::string& api_key) {
    CURL* curl = curl_easy_init();
    std::string response;

    // Encode the chracters since URLs only allow a limited set of characters.
    // Everything else must be percent encoded (like Japanese characters, だってばよ).
    char* encoded = curl_easy_escape(curl, text.c_str(), text.length());

    // Build the message with a bunch of addition.
    std::string postfields = "text=" + std::string(encoded) + "&target_lang=EN-US";
    // Free the encoded text.
    curl_free(encoded); 
    // Create headers.
    struct curl_slist* headers = nullptr; 
    headers = curl_slist_append(headers, ("Authorization: DeepL-Auth-Key " + api_key).c_str());
    // Create object and give it curl and headers.
    Cleanup cl(curl, headers);

    // Use c_str since libcurl is a C library, therefore it needs C-like strings.
    curl_easy_setopt(curl, CURLOPT_URL, "https://api-free.deepl.com/v2/translate");     // Where to send.
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());                     // What to send.
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);                                // Credentials.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);                      // Which function handles received.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);                               // Collect into response.

    // Error check.
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK){
        throw std::runtime_error(curl_easy_strerror(res));
    }

    static std::map<long, std::string> http_code_map = {
        {404, "Resource does not exist or could not be found."},
        {429, "Too many requests in a short period of time."},
        {456, "Free API quota exceeded (500,000 characters)."},
        {500, "Internal server error, please try again later."}
    };

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200){
        if (http_code_map.find(http_code) != http_code_map.end()){
            return std::to_string(http_code) + http_code_map.at(http_code);
        }else{
            return "Error " + std::to_string(http_code) + ":\nVisit"
            "https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status#client_error_responses for more information";
        }

    }

    auto parsed = json::parse(response);

    if (!parsed.contains("translations")){
        return "API key is not provided | Can be set from settings.";
    }
    
    std::string translation = parsed["translations"][0]["text"];
    // println("{}", translation);

    return translation;
}

// IN: std::string text and an optional email (a valid email raises MyMemory's daily limit from 5,000 to 50,000 chars)
// OUT: parsed std::string translation
std::string mymemory_translate(const std::string& text, const std::string& email) {
    CURL* curl = curl_easy_init();
    std::string response;

    // Encode the chracters since URLs only allow a limited set of characters.
    // Everything else must be percent encoded (like Japanese characters, だってばよ).
    char* encoded = curl_easy_escape(curl, text.c_str(), text.length());

    // Build the URL; MyMemory is a simple GET API.
    // langpair is source|target, "ja|en" for Japanese to English.
    std::string url = "https://api.mymemory.translated.net/get?q=" + std::string(encoded) + "&langpair=ja|en";
    curl_free(encoded);

    // Attach the email if the user gave one ("de" stands for "dear email" in their docs).
    if (!email.empty()){
        char* encoded_email = curl_easy_escape(curl, email.c_str(), email.length());
        url += "&de=" + std::string(encoded_email);
        curl_free(encoded_email);
    }

    // Create object and give it curl (no custom headers needed here).
    Cleanup cl(curl, nullptr);

    // Use c_str since libcurl is a C library, therefore it needs C-like strings.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());                   // Where to make the request.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);      // Which function handles received.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);               // Collect into response.

    // Error check.
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK){
        throw std::runtime_error(curl_easy_strerror(res));
    }

    static std::map<long, std::string> http_code_map = {
        {403, "Invalid request or daily quota exceeded."},
        {404, "Resource does not exist or could not be found."},
        {429, "Too many requests in a short period of time."},
        {500, "Internal server error, please try again later."}
    };

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code != 200){
        if (http_code_map.find(http_code) != http_code_map.end()){
            return std::to_string(http_code) + http_code_map.at(http_code);
        }else{
            return "Error " + std::to_string(http_code) + ":\nVisit"
            "https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status#client_error_responses for more information";
        }
    }

    auto parsed = json::parse(response);

    // MyMemory tucks errors inside a 200 response, so check its own status field too.
    // On errors, responseData.translatedText holds the error details instead of a translation.
    if (!parsed.contains("responseData") || parsed["responseData"]["translatedText"].is_null()){
        return "MyMemory returned an unexpected response.";
    }

    std::string translation = parsed["responseData"]["translatedText"];

    if (parsed.contains("responseStatus") && parsed["responseStatus"].is_number() && parsed["responseStatus"].get<int>() != 200){
        return "MyMemory error " + std::to_string(parsed["responseStatus"].get<int>()) + ":\n" + translation;
    }

    return translation;
}

// I have officially scrapped this idea, since there is no OFFICIAL Google Translate API
// MyMemory has been implemented
/* todo:
std::string google_translate(const std::string& text){
    CURL* curl = curl_easy_init();
    std::string response;

    char* encoded = curl_easy_escape(curl, text.c_str(), text.length());

    std::string 
}
*/

// AnkiConnect implementation
std::string anki_get_decks(){
    CURL* curl = curl_easy_init();
    if (!curl){
        throw std::runtime_error("W | curl_easy_init failed | AnkiConnect block");
    }

    std::string read_buffer;
    std::string post_data = R"({"action": "deckNames", "version": 6})";

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8765");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK){
        throw std::runtime_error(curl_easy_strerror(res));
    }

    nlohmann::json response = nlohmann::json::parse(read_buffer);

    if (!response["error"].is_null()){
        throw std::runtime_error("W | AnkiConnect error: " + response["error"].get<std::string>());
    }

    // Separate the deck names with \n
    std::string decks;
    for (const auto& name : response["result"]){
        decks += name.get<std::string>() + "\n";
    }
    if (!decks.empty()){
        decks.pop_back();
    }

    return decks;
}

// Adds a new note (card) to the given deck using AnkiConnect's addNote action.
// AnkiConnect identifies decks by name, so "deck" is a plain string. Uses the
// standard "Basic" note type with "Front" and "Back" fields. Returns the new
// note's ID on success and throws std::runtime_error otherwise.
long long anki_add_note(const std::string& deck, const std::string& front, const std::string& back){
    CURL* curl = curl_easy_init();
    if (!curl){
        throw std::runtime_error("W | curl_easy_init failed | AnkiConnect block");
    }

    std::string read_buffer;

    // Build the request body explicitly to avoid initializer-list ambiguity.
    json note;
    note["deckName"] = deck;
    note["modelName"] = "Basic";
    note["fields"]["Front"] = front;
    note["fields"]["Back"] = back;
    note["tags"] = json::array();

    json payload;
    payload["action"] = "addNote";
    payload["version"] = 6;
    payload["params"]["note"] = note;

    std::string post_data = payload.dump();

    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8765");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK){
        throw std::runtime_error(curl_easy_strerror(res));
    }

    json response = json::parse(read_buffer);

    if (!response["error"].is_null()){
        throw std::runtime_error("W | AnkiConnect error: " + response["error"].get<std::string>());
    }

    return response["result"].get<long long>();
}