#include <FL/fl_draw.H>
#include "include/truncate_label.h"

std::string truncate_label(const std::string& text, int max_width) {
    if (fl_width(text.c_str()) <= max_width) return text;
    std::string truncated = text;
    while (!truncated.empty() && fl_width((truncated + "...").c_str()) > max_width)
        truncated.pop_back();
    return truncated + "...";
}