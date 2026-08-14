#ifdef _WIN32

#include <windows.h>
#include <stdlib.h>
#include <wchar.h>
#include "utf8.h"

char **convert_wargv_to_utf8(int argc, wchar_t *wargv[]) {
    if(argc <= 0 || wargv == NULL) {
        return NULL;
    }

    char **argv = calloc((size_t)argc + 1, sizeof(char *));
    if(argv == NULL) {
        return NULL;
    }

    for(int i = 0; i < argc; ++i) {
        int required = WideCharToMultiByte(
            CP_UTF8,
            0,
            wargv[i],
            -1,
            NULL,
            0,
            NULL,
            NULL
        );

        if(required <= 0) {
            free_utf8_argv(argc, argv);
            return NULL;
        }

        argv[i] = malloc((size_t)required);
        if(argv[i] == NULL) {
            free_utf8_argv(argc, argv);
            return NULL;
        }

        int written = WideCharToMultiByte(
            CP_UTF8,
            0,
            wargv[i],
            -1,
            argv[i],
            required,
            NULL,
            NULL
        );

        if(written <= 0) {
            free_utf8_argv(argc, argv);
            return NULL;
        }
    }

    argv[argc] = NULL;

    return argv;
}


void free_utf8_argv(int argc, char *argv[])
{
    if(argv == NULL) {
        return;
    }

    for(int i = 0; i < argc; ++i) {
        free(argv[i]);
    }

    free(argv);
}

#endif