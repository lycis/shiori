#ifndef _SHIORI_CLI_H
#define _SHIORI_CLI_H

#include <stdbool.h>

void strip_leading_flags(int *argc, char ***argv);
bool has_switch(int argc, char *argv[], const char *sw);

#endif