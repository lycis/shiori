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

