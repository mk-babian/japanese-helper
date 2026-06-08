#include <filesystem>
#include <stdexcept>
#include "include/get_data_dir.h"

#ifdef _WIN32
  #include <windows.h>
  #include <shlobj.h>
#endif

std::filesystem::path get_data_dir(const std::string& app_name) {
    std::filesystem::path base;

#ifdef _WIN32
    wchar_t path[MAX_PATH];
    // Gets %APPDATA% (LOCAL)
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
        base = std::filesystem::path(path);
    } else {
        throw std::runtime_error("Could not retrieve AppData path");
    }
#else
    // Respect XDG spec. fall back to ~/.config
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        base = std::filesystem::path(xdg);
    } else {
        const char* home = std::getenv("HOME");
        if (!home) throw std::runtime_error("Could not retrieve HOME path");
        base = std::filesystem::path(home) / ".config";
    }
#endif

    std::filesystem::path data_dir = base / app_name;

    // Create the directory if it doesn't exist yet
    std::filesystem::create_directories(data_dir);

    return data_dir;
}