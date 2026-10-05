#ifndef SHIORI_TODO_H
#define SHIORI_TODO_H

#include <time.h>

#include "common.h"

// Datatypes

typedef enum {
    OPEN,
    IN_PROGRESS,
    DONE,
    CANCELLED,
    DEFERRED
} todo_status;

enum todo_priority {
    TODO_PRIORITY_NONE,
    TODO_PRIORITY_HIGH,
    TODO_PRIORITY_MEDIUM,
    TODO_PRIORITY_LOW
};

const char *todo_priority_string(enum todo_priority priority);
[[nodiscard]] int parse_todo_priority(const char *value, enum todo_priority *priority);

struct todo {
    enum todo_priority priority;
    char text[DEFAULT_BUFFER_SIZE * 2];
    char topic[DEFAULT_BUFFER_SIZE];
    time_t created;
    time_t due;
    unsigned long long id;
    todo_status status;
};

struct todo_metadata {
    int version;
    unsigned long long last_id;
};

const char *todo_status_icon(todo_status status);
[[nodiscard]] int format_todo_date(time_t timestamp, char *buffer, size_t size);
const char *todo_status_mark(todo_status status);
const char *todo_status_string(todo_status status);
const char *todo_status_simple_icon(todo_status status);

#endif
