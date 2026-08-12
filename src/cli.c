#include <stdbool.h>
#include <string.h>

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