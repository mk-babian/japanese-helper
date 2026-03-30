#pragma once
#include <string>

std::string jisho_lookup(const std::string& word);
std::string deepl_translate(const std::string& text, const std::string& api_key);
