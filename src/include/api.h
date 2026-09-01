#pragma once
#include <string>

#include "app_state.h"

std::string jisho_lookup(const std::string& word, AppState* app);
std::string deepl_translate(const std::string& text, const std::string& api_key);
std::string mymemory_translate(const std::string& text, const std::string& email);
std::string google_translate(const std::string& text);
std::string anki_get_decks();
long long anki_add_note(const std::string& deck, const std::string& front, const std::string& back);
