#include <filesystem>

#include "include/get_exec_path.h"

std::filesystem::path get_executable_path(){
#ifdef _WIN32
    // Windows implementation
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::filesystem::path(path);
#else
    // Linux implementation
    return std::filesystem::read_symlink("/proc/self/exe");
#endif
}