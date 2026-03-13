#include <string>

#include <curl/curl.h>
#include <curl/easy.h>

#include "lib/json.hpp"
using json = nlohmann::json;

#include "include/api.h"

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
