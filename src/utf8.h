#ifndef SHIORI_UTF8_H
#define SHIORI_UTF8_H

#include <wchar.h>

char **convert_wargv_to_utf8(int argc, wchar_t *wargv[]);
void free_utf8_argv(int argc, char *argv[]);
#endif
