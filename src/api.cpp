// For HTTP codes
#include <map>

// CURL
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/urlapi.h>

// JSON
#include "lib/json.hpp"
using json = nlohmann::json;

#include "include/api.h"



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
std::string jisho_lookup(const std::string& word){
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
    curl_slist_append(headers, 
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
        throw std::runtime_error("Jisho returned non-JSON response:\n" + response.substr(0, 200));
    }

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
            return "Error " + std::to_string(http_code) + ":\nVisit https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status#client_error_responses for more information";
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
/* todo:
std::string google_translate(const std::string& text){
    CURL* curl = curl_easy_init();
    std::string response;

    char* encoded = curl_easy_escape(curl, text.c_str(), text.length());

    std::string 
}
*/
