#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "platform.h"
#include "logging.h"
#include "config.h"

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

int run_process(const char *path, const char* working_dir) {
#ifdef _WIN32
    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_info = {0};

    startup_info.cb = sizeof(startup_info);

    char command_line[DEFAULT_BUFFER_SIZE * 2];
    int written = snprintf(command_line, sizeof(command_line), "%s", path);
    if(written < 0 || (size_t)written >= sizeof(command_line)) {
        log_critical("Process command line is too long");
        return -1;
    }

    BOOL success = CreateProcessA(
        NULL,
        command_line,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        working_dir,
        &startup_info,
        &process_info
    );

    if(!success) {
         log_error("Failed starting process '%s' (error %lu).\n", path, GetLastError());
        return -1;
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code;
    if(!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        log_error("Failed reading process exit code (error %lu)\n", GetLastError());
        CloseHandle(process_info.hThread);
        CloseHandle(process_info.hProcess);
        return -1;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return (int)exit_code;
#else
#error not implemented
#endif
}

int run_script_basedir(const char* path) {
    if(path == NULL || path[0] == '\0') {
        return -1;
    }

    char script_path[DEFAULT_BUFFER_SIZE * 2];

    if(get_base_dir_file_path(path, script_path, sizeof(script_path)) != R_OK) {
        log_error("Script path is too long.\n");
        return -1;
    }

#ifdef _WIN32
    if(str_ends_with(script_path, ".bat") || str_ends_with(script_path, ".cmd")) {
        char command[DEFAULT_BUFFER_SIZE * 3];
        int written = snprintf(command, sizeof(command), "cmd.exe /c \"\"%s\"\"", script_path);
        if(written < 0 || (size_t)written >= sizeof(command)) {
            log_error("Script command (%s) is too long.\n", path);
            return -1;
        }

        return run_process(command, g_config.base_dir);
    }
#else
#error not implemented
#endif

    return run_process(script_path, g_config.base_dir);
}