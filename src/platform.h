#ifndef _SHIORI_PLATFORM_H
#define _SHIORI_PLATFORM_H

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <direct.h>

#ifndef access
#define access _access
#endif

#define F_OK 0
#endif

#ifdef __linux__
#include <unistd.h>
#include <locale.h>
#include <langinfo.h>
#endif


char* get_path_separator();
int terminal_enable_utf8(void);
int get_user_home(char* buffer, size_t size);
char* get_current_path(char *buffer, size_t size);
int set_environment_variable(const char* name, const char *value);
int run_script_basedir(const char* path);

#endif