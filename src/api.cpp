#include <stdexcept>
#include <string>
#include <map>

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/urlapi.h>

#include "lib/json.hpp"
using json = nlohmann::json;

#include "include/api.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    out->append((char*)contents, size * nmemb);
    return size * nmemb;    // check if all the data has been dealt with
}

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

std::string jisho_lookup(const std::string& word){
    CURL* curl = curl_easy_init();
    std::string response;

    char* encoded = curl_easy_escape(curl, word.c_str(), word.size());  // convert "illegal" characters to hex format
    std::string url = "https://jisho.org/api/v1/search/words?keyword=" + std::string(encoded);
    curl_free(encoded);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, 
            "User-Agent: Mozilla/5.0 (Windows 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");
    Cleanup cl(curl, headers);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());                   // where to make the request
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);                // what headers to attach
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);       // which function handles incoming data chunks
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);               // where that function should write data into
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK){       // if we had an injury during bicep curls
        throw std::runtime_error(curl_easy_strerror(res));
    }

    if (response.empty() || response[0] != '{'){
        throw std::runtime_error("Jisho returned non-JSON response:\n" + response.substr(0, 200));
    }

    // std::println("{}", response);

    auto parsed = json::parse(response);
    auto data = parsed["data"];
    std::string result;

    if (data.empty()){
        result = "No results found.";
        return result;
    }

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

    char* encoded = curl_easy_escape(curl, text.c_str(), text.length());

    std::string postfields = "text=" + std::string(encoded) + "&target_lang=EN-US";         // build the message (i.e. text=こにちわ&target_lang=EN-US)
    curl_free(encoded);
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: DeepL-Auth-Key " + api_key).c_str());
    Cleanup cl(curl, headers);

    // configure thingamabobs
    curl_easy_setopt(curl, CURLOPT_URL, "https://api-free.deepl.com/v2/translate");     // where to send
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());                     // what to send
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);                                // credentials
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);                       // use chunk collector (minecraft chunks)
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);                               // collect into this string (response)

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
