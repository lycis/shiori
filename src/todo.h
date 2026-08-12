#ifndef _SHIORI_TODO_H
#define _SHIORI_TODO_H

#include <time.h>
#include "common.h"

// Datatypes

typedef enum {OPEN, IN_PROGRESS, DONE} todo_status;

struct todo {
    char text[DEFAULT_BUFFER_SIZE * 2];
    time_t created;
    unsigned long long id;
    todo_status status;
};

struct todo_metadata {
    int version;
    unsigned long long last_id;
};

const char *todo_status_icon(todo_status status);
int format_todo_date(time_t timestamp, char *buffer, size_t size);
const char *todo_status_mark(todo_status status);

#endif