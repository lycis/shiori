#ifndef _SHIORI_COMMAND_H
#define _SHIORI_COMMAND_H

int command_init(int argc, char* argv[]);
int command_config(int argc, char* argv[]);
int command_todo(int argc, char* argv[]);
int command_add(int argc, char* argv[]);
int command_today(int argc, char* argv[]);
int command_topic(int argc, char *argv[]);
int command_capture(int argc, char *argv[]);
int command_tag(int argc, char *argv[]);
int command_console(int argc, char*argv[]);
int command_util(int argc, char*argv[]);

int run_command(char* command, int argc, char* argv[]);

#endif