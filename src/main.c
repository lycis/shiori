#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

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

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#include "common.h"
#include "logging.h"
#include "platform.h"
#include "config.h"
#include "cli.h"
#include "commands.h"
#include "hooks.h"
#include "utf8.h"

// --------------------- Prototypes
int run_command(char* command, int argc, char* argv[]);
// --------------------- Prototypes

// --------------------- GLobals
bool g_debug_enabled = false;
// ---------------------

int command_console(int argc, char* argv[]) {

    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {
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
    if(strcmp(command, "version") == 0) {
         printf("%s %s\n", APP_NAME, APP_VERSION);
         print_compiler();
         print_platform();
         print_architecture();
         print_c_standard();
         return R_OK;
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
        printf("  %-16s %s\n", "capture","Interactively capture notes and todos");
        printf("  %-16s %s\n", "topic",  "Browse notes by topic");
        printf("  %-16s %s\n", "todo",   "Manage your todos and tasks");
        printf("  %-16s %s\n", "today",  "Your overview for the current day");
        printf("  %-16s %s\n", "console","Start the interactive console");
        printf("  %-16s %s\n", "help",   "Show this help");
        printf("  %-16s %s\n", "version","Display current version information");
        return R_OK;
    } else if(strcmp(command, "init") == 0) {
        return command_init(argc, argv);
    }

    if(read_config_file() != R_OK) {
        exit(SHIORI_EXIT_CONFIG_ERROR);
    }

    int rc = -1;
    if(strcmp(command, "config") == 0) {
        rc = command_config(argc, argv);
    } else if(strcmp(command, "console") == 0) {
        rc = command_console(argc, argv);
    } else if(strcmp(command, "add") == 0) {
        rc = command_add(argc, argv);
    } else if(strcmp(command, "todo") == 0) {
        rc = command_todo(argc, argv);
    } else if(strcmp(command, "today") == 0) {
        rc = command_today(argc, argv);
    } else if(strcmp(command, "topic") == 0) {
        rc = command_topic(argc, argv);
    } else if(strcmp(command, "capture") == 0) {
        rc = command_capture(argc, argv);
    } else if(strcmp(command, "tag") == 0) {
        rc = command_tag(argc, argv);
    } else {
        log_error("Unknown command: %s\n", command);
        return R_ERROR;
    }

    // call after command hook
    if(g_config.hooks.after_command[0] != '\0') {
        hook_after_command(command, argc, argv);
    }
    
    return rc;
} 


int shiori_main(int argc, char *argv[]) {
    terminal_enable_utf8();

    if(argc > 1) {

        // throw away the executable name
        argc--;
        argv++;

        // enable debug log
        if(has_switch(argc, argv, "--debug", true)) {
            g_debug_enabled = true;
            log_debug("Debug logging enabled\n");
        }

        // run any other commands
        strip_leading_flags(&argc, &argv);

        if(argc == 0) {
            log_error("No command provided.\n");
            return SHIORI_EXIT_NO_COMMAND;
        }

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

#ifdef _WIN32

int wmain(int argc, wchar_t *wargv[]) {
    char **argv = convert_wargv_to_utf8(argc, wargv);

    if(argv == NULL) {
        return SHIORI_EXIT_COMMAND_FAILED;
    }

    int result = shiori_main(argc, argv);

    free_utf8_argv(argc, argv);

    return result;
}

#else

int main(int argc, char *argv[])
{
    return shiori_main(argc, argv);
}

#endif

