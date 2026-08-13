#ifndef _SHIORI_NOTE_H
#define _SHIORI_NOTE_H

#include <time.h>
#include <stddef.h>
#include "common.h"

struct note {
    char text[DEFAULT_BUFFER_SIZE * 2];
    char topic[DEFAULT_BUFFER_SIZE];
    time_t created;
};

struct note_list {
    struct note *items;
    size_t count;
    size_t capacity;
};

void note_list_init(struct note_list *list);
void note_list_free(struct note_list *list);

int note_list_add(
    struct note_list *list,
    const struct note *item
);

int read_notes_for_date(const char *filename, const time_t date, struct note_list *notes);

#endif