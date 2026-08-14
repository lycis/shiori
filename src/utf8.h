#ifndef _SHIORI_UTF_H
#define _SHIORI_UTF_H
char **convert_wargv_to_utf8(int argc, wchar_t *wargv[]);
void free_utf8_argv(int argc, char *argv[]);
#endif