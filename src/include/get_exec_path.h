#include <filesystem>

#if defined(_WIN32)
    #include <windows.h>
#endif

std::filesystem::path get_executable_path();