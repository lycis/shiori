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
#include <direct.h>
#define access _access
#define F_OK 0
#endif

#ifdef __linux__
#include <unistd.h>
#include <locale.h>
#include <langinfo.h>
#endif

// --------------------- Prototypes
int run_command(char* command, int argc, char* argv[]);
// --------------------- Prototypes

// --------------------- GLobals
struct configuration {
    int version;
    char base_dir[4096];
};

struct configuration g_config;
bool g_debug_enabled = false;

// --------------------- GLobals

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

void log_debug(const char* fmt, ...) {
    if(!g_debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    char buffer[DEFAULT_BUFFER_SIZE];
    sprintf(buffer, "[debug] %s", fmt);
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

void strip_leading_flags(int *argc, char ***argv)
{
    while (*argc > 0) {
        char *arg = (*argv)[0];

        if (arg[0] != '-' || arg[1] == '\0') {
            break;
        }

        (*argv)++;
        (*argc)--;
    }
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

char* get_current_path(char *buffer, size_t size) {
    #ifdef _WIN32
    char* ptr = _getcwd(buffer, (int) size);
    #else
    char* ptr =  getcwd(buffer, size);
    #endif

    return ptr;
}

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
        } else if(strcmp(key, "base_dir") == 0) {
            char lc = value[strlen(value)-1];
            if(lc == '\\' || lc == '/') value[strlen(value)-1] = '\0';
            strcpy_s(g_config.base_dir, sizeof(g_config.base_dir), value);
        }
    }

    fclose(config_file);
    
    return R_OK;
}

bool has_switch(int argc, char *argv[], const char *sw)
{
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--") == 0) {
            break;
        }

        if (argv[i][0] != '-' || argv[i][1] == '\0') {
            break;
        }

        if (strcmp(argv[i], sw) == 0) {
            return true;
        }
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

    // by default the base dir for keeping notes is the current work dir
    char buffer[DEFAULT_BUFFER_SIZE];
    get_current_path(buffer, DEFAULT_BUFFER_SIZE);
    fprintf(config_file, "base_dir: %s\n", buffer);

    fclose(config_file);

    log_success(".scratch config file created in current directory\n");
    return 0;
}

int command_config(int argc, char* argv[]) {
    if(argc < 1) {
        log_error("No config command provided. Please provide a config command.\n");
        return R_ERROR;
    }

    if(has_switch(argc, argv, "--help") || has_switch(argc, argv, "-h")) {
        printf("Allows you to view and manage the config file.\n");
        printf("\n");
        printf("Available config commands:\n");
        printf("  show: Show the current config version\n");
        return R_OK;
    }

    if(strcmp(argv[0], "show") == 0) {
        printf("version: %d\n", g_config.version);
        printf("base_dir: %s\n", g_config.base_dir);
    } else {
        log_error("Unknown config command. See --help\n", argv[0]);
        return R_ERROR;
    }

    return R_OK;
}

int command_console(int argc, char* argv[]) {

    if(has_switch(argc, argv, "--help") || has_switch(argc, argv, "-h")) {
        printf("Starts an interactive console mode.\n");
        printf("\n");
        printf("You can enter scratch commands directly in the console. This helps as you do not have to run `scratch <command>` all the time. Useful if you want to work continuously.");
        return R_OK;
    }

    log_info("Starting interactive console mode\n");
    printf("Type 'exit' or 'quit' to exit the console.\n");

    char input[DEFAULT_BUFFER_SIZE];
    printf("scratch> ");
    while(fgets(input, sizeof(input), stdin) != NULL) {
        char *command = trim(input);
        if(strlen(command) == 0) {
            continue;
        }

        if(strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            log_info("Exiting console mode\n");
            break;
        }

        // split command into arguments
        char *argv[DEFAULT_BUFFER_SIZE];
        int argc = 0;
        char *context = NULL;
        char *token = strtok_s(command, " ", &context);
        while(token != NULL && argc < DEFAULT_BUFFER_SIZE) {
            argv[argc++] = token;
            token = strtok_s(NULL, " ", &context);
        }

        run_command(argv[0], argc - 1, &argv[1]);
        printf("scratch> ");
    }

    return R_OK;
}

int run_command(char* command, int argc, char* argv[]) {
    if(read_config_file() != R_OK) {
        exit(EXIT_CONFIG_ERROR);
    }

    if(strcmp(command, "config") == 0) {
        return command_config(argc, argv);
    } else if (strcmp(command, "console") == 0) {
        return command_console(argc, argv);
    } else {
        log_error("Unknown command: %s\n", command);
        return R_ERROR;
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    terminal_enable_utf8();

    if(argc > 1) {

        // throw away the executable name
        argc--;
        argv++;

        // check for init command
        if(strcmp(argv[1], "init") == 0) {
            strip_leading_flags(&argc, &argv);
            if(command_init(--argc, &argv[1]) != R_OK) {
                return EXIT_COMMAND_FAILED;
            }
            return EXIT_SUCCESS;
        }

        // enable debug log
        if(has_switch(argc, argv, "--debug")) {
            g_debug_enabled = true;
            log_debug("Debug logging enabled\n");
        }

        // run any other commands
        strip_leading_flags(&argc, &argv);

        char command[DEFAULT_BUFFER_SIZE];
        strcpy_s(command, sizeof(command), argv[0]);
        argc--;
        argv++;

        if(run_command(command, argc, argv) != R_OK) {
            return EXIT_COMMAND_FAILED;
        }
        return EXIT_SUCCESS;
    } else {
        log_error("No command provided. Please provide a command.\n");
        return EXIT_NO_COMMAND;
    }

    return EXIT_SUCCESS;
}