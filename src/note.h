#ifndef SHIORI_NOTE_H
#define SHIORI_NOTE_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

struct note {
    char id[32];
    char text[DEFAULT_BUFFER_SIZE * 2];
    char topic[DEFAULT_BUFFER_SIZE];
    time_t created;
};

struct note_list {
    struct note *items;
    size_t count;
    size_t capacity;
};

struct notes_metadata {
    unsigned int version;
};

void note_list_init(struct note_list *list);
void note_list_free(struct note_list *list);
[[nodiscard]] int note_list_add(struct note_list *list, const struct note *item);
struct note *note_list_find_by_id(const struct note_list *list, const char *id);
[[nodiscard]] int note_list_remove_by_id(struct note_list *list, const char *id);

[[nodiscard]] int read_notes_for_date(const char *filename, const time_t date, struct note_list *notes);
[[nodiscard]] int read_notes(const char *filename, struct note_list *list);
[[nodiscard]] int create_note_from_markdown(const char *markdown, time_t created, struct note *item);
[[nodiscard]] int read_notes_metadata(const char *filename, struct notes_metadata *md);
[[nodiscard]] int write_note(FILE *file, const struct note *note);
[[nodiscard]] int rewrite_notes(struct note_list *notes, struct notes_metadata *md, bool allow_note_removal);
int restore_notes_backup(void);

#endif
