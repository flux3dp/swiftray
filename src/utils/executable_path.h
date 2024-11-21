#include <string>
#include <filesystem>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#include <limits.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h> // Add this header for _NSGetExecutablePath
#endif

std::string get_executable_path() {
    #if defined(_WIN32)
        wchar_t path[MAX_PATH] = { 0 };
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring ws(path);
        return std::string(ws.begin(), ws.end());
    #elif defined(__linux__)
        char result[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
        return std::string(result, (count > 0) ? count : 0);
    #elif defined(__APPLE__)
        char result[PATH_MAX];
        uint32_t size = sizeof(result);
        if (_NSGetExecutablePath(result, &size) == 0) {
            char real_path[PATH_MAX];
            if (realpath(result, real_path) != NULL) {
                return std::string(real_path);
            }
        }
        return std::string();
    #else
        #error Unsupported platform
    #endif
}

// Get only the directory (without executable name)
std::string get_executable_dir() {
    std::filesystem::path full_path(get_executable_path());
    return full_path.parent_path().string();
}