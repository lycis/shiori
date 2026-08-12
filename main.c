#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>

#if defined(_WIN32)
    #define HAVE_FOPEN_S 1
#elif defined(__STDC_LIB_EXT1__)
    #define HAVE_FOPEN_S 1
#else
    #define HAVE_FOPEN_S 0
#endif

#ifndef HAVE_FOPEN_S
    #error "fopen_s is not available on this platform"
#endif 

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define R_OK 0
#define R_ERROR 1

#define DEFAULT_BUFFER_SIZE 2048

#define EXIT_SUCCESS 0
#define EXIT_CONFIG_ERROR 1
#define EXIT_NO_COMMAND 2
#define EXIT_COMMAND_FAILED 3

#define CONFIG_VERSION 1

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define access _access
#define F_OK 0
#endif

#ifdef __linux__
#include <unistd.h>
#include <locale.h>
#include <langinfo.h>
#endif

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[DEFAULT_BUFFER_SIZE];
    sprintf(buffer, "❌ %s", fmt);
    vfprintf(stderr, buffer, args);
    va_end(args);
}

void log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[DEFAULT_BUFFER_SIZE];
    sprintf(buffer, "ℹ️ %s", fmt);
    vfprintf(stdout, buffer, args);
    va_end(args);
}

void log_success(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buffer[DEFAULT_BUFFER_SIZE];
    sprintf(buffer, "✔️ %s", fmt);
    vfprintf(stdout, buffer, args);
}

char *trim(char *str)
{
    while (isspace((unsigned char)*str)) {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;

    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';
    return str;
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

    return strcmp(encoding, "UTF-8") == 0 ||
           strcmp(encoding, "UTF8") == 0;
    #endif
}

void get_user_home(char* buffer, size_t size) {
    #ifdef _WIN32
    if(_dupenv_s(&buffer, &size, "USERPROFILE") != 0) {
        log_error("Error getting USERPROFILE environment variable\n");
        exit(EXIT_FAILURE);
    }
    #else
    #error "get_user_home is only implemented for Windows"
    #endif
}

struct configuration {
    int version;
};

struct configuration g_config;

int read_config_file() {
    char* cf_file_path = ".scratch";
    
    // first check current directory
    if(access(cf_file_path, F_OK) != 0) {
        char user_home[DEFAULT_BUFFER_SIZE];
        get_user_home(user_home, sizeof(user_home));

        char buffer[DEFAULT_BUFFER_SIZE];
        sprintf(buffer, "%s/.scratch", user_home);
        if(access(buffer, F_OK) != 0) {
            log_error(".scratch config file not found. please run `scratch init` first.\n");
            return R_ERROR;
        }

        cf_file_path = buffer;
    }

    FILE *config_file = NULL;
    errno_t err = fopen_s(&config_file, cf_file_path, "r");
    if(err != 0 || config_file == NULL) {
        log_error("Error opening .scratch file\n");
        return R_ERROR;
    }

    char line[DEFAULT_BUFFER_SIZE];
    int lnr = 0;
    while(fgets(line, sizeof(line), config_file) != NULL) {
        lnr++;
        if(line[0] == '#') continue; // comment
        if(strlen(line) == 0 || line[0] == '\n') continue; // empty line

        if(strstr(line, ":") == NULL) {
            log_error("Invalid config entry at line %d\n", lnr);
            fclose(config_file);
            return R_ERROR;
        }

        char *line_ptr = line;

        char *key = strtok_s(line, ":", &line_ptr);
        if(key == NULL) {
            log_error("Invalid config entry at line %d\n", lnr);
            fclose(config_file);
            return R_ERROR;
        }
        key = trim(key);

        char *value = strtok_s(NULL, "=", &line_ptr);
        if(value == NULL) {
            value = "";
        }
        value = trim(value);

        if(strcmp(key, "version") == 0) {
            g_config.version = atoi(value);
            if(g_config.version == 0) {
                log_error("Invalid config version at line %d\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        }
    }

    fclose(config_file);
    
    return R_OK;
}

bool has_switch(int argc, char* argv[], const char *sw) {
    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], sw) == 0) return true;
    }
    return false;
}

int command_init(int argc, char* argv[]) {
    bool config_exists = access(".scratch", F_OK) == 0;

    // check if there is already a config
    if(!has_switch(argc, argv, "--reinit")) {
        if(config_exists) {
            log_error(".scratch already exists in current directory\n");
            printf("If you want to reinitialize, please delete the existing .scratch file first or specify --reinit.\n");
            return R_ERROR;
        }
    }

    if(config_exists) {
        log_info("Reinitializing .scratch config in current directory.\n");
    }

    FILE *config_file = NULL;
    errno_t err = fopen_s(&config_file, ".scratch", "w");
    if(err != 0 || config_file == NULL) {
        log_error("Error creating .scratch file in current directory\n");
        return R_ERROR;
    }

    // write comment with generation date to file
    time_t now = time(NULL);
    char date_str[26]; // ctime_s requires a buffer of at least 26 bytes
    ctime_s(date_str, 26, &now);
    fprintf(config_file, "# Initialized: %s", date_str);

    // write up to date config file version
    fprintf(config_file, "version: %d\n", CONFIG_VERSION);

    fclose(config_file);

    log_success(".scratch config file created in current directory\n");
    return 0;
}

int command_config(int argc, char* argv[]) {
    if(argc < 1) {
        log_error("No config command provided. Please provide a config command.\n");
        return R_ERROR;
    }

    if(strcmp(argv[0], "show") == 0) {
        printf("version: %d\n", g_config.version);
    } else {
        log_error("Unknown config command: %s\n", argv[0]);
        return R_ERROR;
    }

    return R_OK;
}

int run_command(char* command, int argc, char* argv[]) {
    if(read_config_file() != R_OK) {
        exit(EXIT_CONFIG_ERROR);
    }

    if(strcmp(command, "config") == 0) {
        return command_config(--argc, &argv[1]);
    } else {
        log_error("Unknown command: %s\n", command);
        return R_ERROR;
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    terminal_enable_utf8();

    if(argc > 1) {
        if(strcmp(argv[1], "init") == 0) {
            if(command_init(--argc, &argv[1]) != R_OK) {
                return EXIT_COMMAND_FAILED;
            }
            return EXIT_SUCCESS;
        }

        if(run_command(argv[1], --argc, &argv[1]) != R_OK) {
            return EXIT_COMMAND_FAILED;
        }
        return EXIT_SUCCESS;
    } else {
        log_error("No command provided. Please provide a command.\n");
        return EXIT_NO_COMMAND;
    }

    return EXIT_SUCCESS;
}