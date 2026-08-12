#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>

#if defined(_WIN32)
    #define HAVE_FOPEN_S 1
#elif defined(__STDC_LIB_EXT1__)
    #define HAVE_FOPEN_S 1
#else
    #define HAVE_FOPEN_S 0
#endif

#if !HAVE_FOPEN_S
    #error "fopen_s is not available on this platform"
#endif 

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h>

#define R_OK 0
#define R_ERROR 1

#define DEFAULT_BUFFER_SIZE 2048

#define SHIORI_EXIT_SUCCESS 0
#define SHIORI_EXIT_CONFIG_ERROR 1
#define SHIORI_EXIT_NO_COMMAND 2
#define SHIORI_EXIT_COMMAND_FAILED 3

#define CONFIG_FILE_NAME ".shiori"
#define CONFIG_VERSION 1
#define APP_NAME "shiori"
#define APP_VERSION "0.1.0"

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

static void log_v(FILE *stream, const char *prefix, const char *fmt, va_list args)
{
    fputs(prefix, stream);
    vfprintf(stream, fmt, args);
}

void log_error(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "❌ ", fmt, args);
    va_end(args);
}

void log_critical(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "🤯 ", fmt, args);
    va_end(args);
}

void log_warning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stderr, "⚠️ ", fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "ℹ️ ", fmt, args);
    va_end(args);
}

void log_success(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_v(stdout, "✔️ ", fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...)
{
    if (!g_debug_enabled) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    log_v(stdout, "[debug] ", fmt, args);
    va_end(args);
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

    return strcmp(encoding, "UTF-8") == 0 ||
           strcmp(encoding, "UTF8") == 0;
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

int read_config_file() {
    char* cf_file_path = CONFIG_FILE_NAME;
    char config_path[DEFAULT_BUFFER_SIZE];

    if(access(CONFIG_FILE_NAME, F_OK) == 0) {
        strcpy_s(config_path, sizeof(config_path), CONFIG_FILE_NAME);
    } else {
        char user_home[DEFAULT_BUFFER_SIZE];

        if(get_user_home(user_home, sizeof(user_home)) != R_OK) {
            log_error("Could not determine user home directory.\n");
            return R_ERROR;
        }

        snprintf(config_path, sizeof(config_path),"%s%s%s", user_home, get_path_separator(), CONFIG_FILE_NAME);

        if(access(config_path, F_OK) != 0) {
            log_error("%s config file not found. Please run `%s init` first.\n", CONFIG_FILE_NAME, APP_NAME);
            return R_ERROR;
        }
    }

    FILE *config_file = NULL;
    errno_t err = fopen_s(&config_file, cf_file_path, "r");
    if(err != 0 || config_file == NULL) {
        log_error("Error opening %S file\n", CONFIG_FILE_NAME);
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

        char *colon = strchr(line, ':');

        if(colon == NULL) {
            log_error("Invalid config entry at line %d\n", lnr);
            return R_ERROR;
        }

        *colon = '\0';

        char *key = trim(line);
        char *value = trim(colon + 1);

        if(strcmp(key, "version") == 0) {
            g_config.version = atoi(value);
            if(g_config.version == 0) {
                log_error("Invalid config version at line %d\n", lnr);
                fclose(config_file);
                return R_ERROR;
            }
        } else if(strcmp(key, "base_dir") == 0) {
            size_t len = strlen(value);
            if(len > 0) {
                char lc = value[strlen(value)-1];
                if(lc == '\\' || lc == '/') value[strlen(value)-1] = '\0';
                strcpy_s(g_config.base_dir, sizeof(g_config.base_dir), value);
            } else {
                log_error("invalid configuration (line %d): Empty base directory is not permitted.\n", lnr);
            }
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
    bool config_exists = access(CONFIG_FILE_NAME, F_OK) == 0;

    // check if there is already a config
    if(!has_switch(argc, argv, "--reinit")) {
        if(config_exists) {
            log_error("%s already exists in current directory\n", CONFIG_FILE_NAME);
            printf("If you want to reinitialize, please delete the existing %s file first or specify --reinit.\n", CONFIG_FILE_NAME);
            return R_ERROR;
        }
    }

    if(config_exists) {
        log_info("Reinitializing config in current directory.\n");
    }

    FILE *config_file = NULL;
    errno_t err = fopen_s(&config_file, CONFIG_FILE_NAME, "w");
    if(err != 0 || config_file == NULL) {
        log_error("Error creating config file in current directory\n");
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

    log_success("%S config file created in current directory\n", CONFIG_FILE_NAME);
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
        printf("You can enter %s commands directly in the console. This helps as you do not have to run `%s <command>` all the time. Useful if you want to work continuously.", APP_NAME, APP_NAME);
        return R_OK;
    }

    log_info("Starting interactive console mode\n");
    printf("Type 'exit' or 'quit' to exit the console.\n");

    char input[DEFAULT_BUFFER_SIZE];
    printf("%s 🦊> ", APP_NAME);
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
        printf("%s 🦊> ", APP_NAME);
    }

    return R_OK;
}

bool is_heading(const char *line) {
    return line[0] == '#' && line[1] == ' ';
}

int build_daily_heading(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm local_time;
    if(localtime_s(&local_time, &now) != 0) {
        log_error("Failed to get local time.");
        return R_ERROR;
    }

    char date_str[11];
    if(strftime(date_str, sizeof(date_str), "%Y-%m-%d", &local_time) == 0) {
        log_error("Feild to format local date.");        
        return R_ERROR;
    }

    int written = snprintf(buffer, size, "# %s", date_str);
    if(written < 0 || written >= (int)size) {
        log_critical("Daily heading buffer too small.");
        return R_ERROR;
    }

    return R_OK;
}

void write_note_to_file(int argc, char* argv[], FILE* note) {
    fprintf(note, "* ");
    for(int i=0; i<argc; ++i) {
        fprintf(note, "%s", argv[i]);
        if(i != argc-1) fprintf(note, " ");
    }
    fprintf(note, "\n");
    return;
}

bool heading_matches(const char *line, const char *heading){
    size_t len = strlen(heading);

    return strncmp(line, heading, len) == 0 &&
           (line[len] == '\n' ||
            line[len] == '\r' ||
            line[len] == '\0');
}

int create_file_if_not_exists(char* fname) {
    log_debug("Checking if file '%s' exists.\n", fname);

    if(access(fname, F_OK) == 0) return R_OK;

    log_debug("File does not exist. Creating it now.\n");
    FILE *f = NULL;
    log_debug("Opening daily note at: %s\n", fname);
    errno_t err = fopen_s(&f, fname, "w");
    if(err != 0  || f == NULL) {
        log_error("Failed creating file: %s\n", fname);
        return R_ERROR;
    }

    log_success("Created %s\n", fname);
    fclose(f);
    return R_OK;
}

int command_add(int argc, char* argv[]) {
    log_debug("Adding a new note.\n");

    // construct our daily heading
    char heading[32];
    if(build_daily_heading(heading, sizeof(heading)) != R_OK) {
        return R_ERROR;
    }
    log_debug("Daily heading = %s\n", heading);

    // if NOTES.MD does not exist we need to create it
    char notes_md[DEFAULT_BUFFER_SIZE];
    sprintf(notes_md, "%s%s%s", g_config.base_dir, get_path_separator(), "NOTES.md");
    if(create_file_if_not_exists(notes_md) != R_OK)
        return R_ERROR;

    // we are going to read from NOTES.md to a temporary file
    FILE *source = NULL;
    log_debug("Opening daily note at: %s\n", notes_md);
    errno_t err = fopen_s(&source, notes_md, "r");
    if(err != 0  || source == NULL) {
        log_error("Failed opening NOTES.md file.");
        return R_ERROR;
    }

    FILE *temp = NULL;
    char temp_path[DEFAULT_BUFFER_SIZE];
    sprintf(temp_path, "%s%s%s", g_config.base_dir, get_path_separator(), "NOTES.md.tmp");
    log_debug("Opening temporary note file for writing at: %s\n", temp_path);
    err = fopen_s(&temp, temp_path, "w");
    if(err != 0  || temp == NULL) {
        log_error("Failed opening NOTES.md.tmp file.\n");
        fclose(source);
        return R_ERROR;
    }

    bool found_heading = false;
    bool note_written = false;

    char line[DEFAULT_BUFFER_SIZE];
    while(fgets(line, sizeof(line), source) != NULL) {
        if(!found_heading) {
            if(heading_matches(line, heading)) {
                log_debug("Found today's heading.\n");
                found_heading = true;
            }
        } else if(!note_written && is_heading(line)) {
            log_debug("Writing note into today's block\n");
            write_note_to_file(argc, argv, temp);
            note_written = true;
        }

        fputs(line, temp);
    }

    if(found_heading && !note_written) {
        log_debug("Note added at end of file to existing heading.\n");
        write_note_to_file(argc, argv, temp);
    }
    if(!found_heading) {
        log_debug("Day not found yet. Adding with note.\n");
        fprintf(temp, "\n%s\n", heading);
        write_note_to_file(argc, argv, temp);
    }

    log_debug("Closing file handles.\n");
    fclose(source);
    fclose(temp);

    log_debug("backing up NOTES.md\n");
    char nb_fname[2 * DEFAULT_BUFFER_SIZE];
    sprintf(nb_fname, "%s.bak", notes_md);

    if(access(nb_fname, F_OK) == 0) {
        log_debug("Removing existing notes backup.\n");
        if(remove(nb_fname) != 0) {
            log_error("Could not remove previous backup: %s\n", nb_fname);
            return R_ERROR;
        }
    }

    if(rename(notes_md, nb_fname) != 0) {
        log_error("Failed to backup NOTES.md\n");
        return R_ERROR;
    }

    log_debug("Renaming temporary file to NOTES.md\n");
    if(rename(temp_path, notes_md) != 0) {
        log_error("Failed to rename temporary notes file.\n");

        log_debug("Restoring NOTES.md from backup.\n");
        if (rename(nb_fname, notes_md) != 0) {
            log_critical("Failed to restore NOTES.md from backup.\n");
        }

        return R_ERROR;
    }

    log_debug("Deleting old NOTES.md.bak\n");
    if(remove(nb_fname) != 0) {
        log_warning("Failed to delete old %s\n", nb_fname);
    }

    log_success("Note added.\n");
    return R_OK;
}

void print_compiler(void)
{
#if defined(__clang__)
    printf("compiler: clang %d.%d.%d\n",
           __clang_major__,
           __clang_minor__,
           __clang_patchlevel__);

#elif defined(__GNUC__)
    printf("compiler: gcc %d.%d.%d\n",
           __GNUC__,
           __GNUC_MINOR__,
           __GNUC_PATCHLEVEL__);

#elif defined(_MSC_VER)
    printf("compiler: msvc %d\n", _MSC_VER);

#else
    printf("compiler: unknown\n");
#endif
}

void print_platform(void)
{
#if defined(_WIN32)
    printf("platform: windows");
#elif defined(__linux__)
    printf("platform: linux");
#elif defined(__APPLE__)
    printf("platform: macos");
#else
    printf("platform: unknown");
#endif
}

void print_architecture(void) {
#if defined(__x86_64__) || defined(_M_X64)
    printf(" x86_64");
#elif defined(__aarch64__) || defined(_M_ARM64)
    printf(" arm64");
#elif defined(__i386__) || defined(_M_IX86)
    printf(" x86");
#else
    printf(" unknown-arch");
#endif

    printf("\n");
}

void print_c_standard(void)
{
#if defined(__STDC_VERSION__)
    #if __STDC_VERSION__ >= 202311L
        printf("c standard: C23\n");
    #elif __STDC_VERSION__ >= 201710L
        printf("c standard: C17\n");
    #elif __STDC_VERSION__ >= 201112L
        printf("c standard: C11\n");
    #elif __STDC_VERSION__ >= 199901L
        printf("c standard: C99\n");
    #else
        printf("c standard: pre-C99\n");
    #endif
#else
    printf("c standard: C90\n");
#endif
}

int run_command(char* command, int argc, char* argv[]) {
    if(read_config_file() != R_OK) {
        exit(SHIORI_EXIT_CONFIG_ERROR);
    }

    if(strcmp(command, "config") == 0) {
        return command_config(argc, argv);
    } else if(strcmp(command, "console") == 0) {
        return command_console(argc, argv);
    } else if(strcmp(command, "add") == 0) {
        return command_add(argc, argv);
    } else if(strcmp(command, "version") == 0) {
         printf("%s %s\n", APP_NAME, APP_VERSION);
         print_compiler();
         print_platform();
         print_architecture();
         print_c_standard();
    } else if(strcmp(command, "help") == 0) {
        printf("%s is a console scratchpad tool that helps you maintain thoughts, quick notes and todos in a quick fire-and-forget fashion.", APP_NAME);
        printf("usage: %s [options] <command> [options] [subcommand] ...\n", APP_NAME);
        printf("\n");
        printf("Options:\n");
        printf("  %-16s %s\n", "--debug", "Show debug and plumbing output.");
        printf("\n");
        printf("Available commands:\n");
        printf("  %-16s %s\n", "init",   "Initialize a new configuration");
        printf("  %-16s %s\n", "config", "Show or modify configuration");
        printf("  %-16s %s\n", "add",    "Add a new note or thought to the day");
        printf("  %-16s %s\n", "help",   "Show this help");
        printf("  %-16s %s\n", "version","Display current version information");
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
        if(strcmp(argv[0], "init") == 0) {
            strip_leading_flags(&argc, &argv);
            if(command_init(--argc, &argv[1]) != R_OK) {
                return SHIORI_EXIT_COMMAND_FAILED;
            }
            return SHIORI_EXIT_SUCCESS;
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
            return SHIORI_EXIT_COMMAND_FAILED;
        }
        return SHIORI_EXIT_SUCCESS;
    } else {
        log_error("No command provided. Please provide a command.\n");
        return SHIORI_EXIT_NO_COMMAND;
    }

    return SHIORI_EXIT_SUCCESS;
}