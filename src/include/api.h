#pragma once
#include <string>

std::string jisho_lookup(const std::string& word);
std::string deepl_translate(const std::string& text, const std::string& api_key);
std::string mymemory_translate(const std::string& text, const std::string& email);
std::string google_translate(const std::string& text);
