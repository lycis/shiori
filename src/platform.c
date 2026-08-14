#include <stdlib.h>
#include "common.h"
#include "platform.h"

char* get_path_separator() {
    #ifdef _WIN32
    return "\\";
    #else
    return "/";
    #endif
}

int terminal_enable_utf8(void) {
    #ifdef _WIN32
    if(GetConsoleOutputCP() == CP_UTF8) {
        return R_OK;
    }

    return SetConsoleOutputCP(CP_UTF8) ? R_OK : R_ERROR;
    #else
     setlocale(LC_CTYPE, "");

    const char *encoding = nl_langinfo(CODESET);

    bool is_utf8 = strcmp(encoding, "UTF-8") == 0 || strcmp(encoding, "UTF8") == 0;
    return is_utf8 ? R_OK : R_ERROR;
    #endif
}

int get_user_home(char* buffer, size_t size) {
    #ifdef _WIN32
    size_t required = 0;

    if (getenv_s(&required, buffer, size, "USERPROFILE") != 0 ||
        required == 0) {
        return R_ERROR;
    }

    return R_OK;
    #else
    #error "get_user_home is only implemented for Windows"
    #endif
}

char* get_current_path(char *buffer, size_t size) {
    #ifdef _WIN32
    char* ptr = _getcwd(buffer, (int) size);
    #else
    char* ptr =  getcwd(buffer, size);
    #endif

    return ptr;
}

int set_environment_variable(const char* name, const char *value) {
    #ifdef _WIN32
    errno_t err = _putenv_s(name, value);
    if(err != 0) return R_ERROR;
    return R_OK;
    #else
    #error not implemented
    #endif
}

int run_command(const char *command) {
    // TODO replace with a better option than system
    return system(command);
}
