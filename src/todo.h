#ifndef _SHIORI_TODO_H
#define _SHIORI_TODO_H

#include <time.h>
#include "common.h"

// Datatypes
struct todo {
    char text[DEFAULT_BUFFER_SIZE * 2];
    time_t created;
    unsigned long long id;
    enum {OPEN, IN_PROGRESS, DONE} status;
};

struct todo_metadata {
    int version;
    unsigned long long last_id;
};
#endif