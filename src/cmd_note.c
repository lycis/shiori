#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "commands.h"
#include "logging.h"
#include "note.h"
#include "cli.h"

int command_note(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    log_error("Please specify a todo command. Refer to `note help` if required.");

    return R_ERROR;
}

#define MAX_NOTE_TAGS 32
#define MAX_TAG_LENGTH 128

struct tag_list {
    char items[MAX_NOTE_TAGS][MAX_TAG_LENGTH];
    size_t count;
};

int extract_note_tags(const struct note *note, struct tag_list *tags) {
    if(note == NULL || tags == NULL) {
        return R_ERROR;
    }

    tags->count = 0;

    const char *p = note->text;

    while(*p != '\0') {
        if(*p != '#') {
            p++;
            continue;
        }

        const char *start = p + 1;

        if(*start == '\0') {
            break;
        }

        size_t len = 0;

        while(start[len] != '\0' &&
              !isspace((unsigned char)start[len]) &&
              start[len] != ',' &&
              start[len] != '.' &&
              start[len] != ';' &&
              start[len] != ':' &&
              start[len] != '!' &&
              start[len] != '?' &&
              start[len] != ')' &&
              start[len] != ']' &&
              start[len] != '}') {
            len++;
        }

        if(len == 0) {
            p++;
            continue;
        }

        /*
         * Shiori's own metadata is not a user tag.
         */
        if(len >= 7 && strncmp(start, "shiori/", 7) == 0) {
            p = start + len;
            continue;
        }

        if(len >= MAX_TAG_LENGTH) {
            log_warning("Ignoring tag that is too long.\n");
            p = start + len;
            continue;
        }

        if(tags->count >= MAX_NOTE_TAGS) {
            log_warning("Maximum number of note tags reached.\n");
            break;
        }

        memcpy(
            tags->items[tags->count],
            start,
            len
        );

        tags->items[tags->count][len] = '\0';
        tags->count++;

        p = start + len;
    }

    return R_OK;
}

int command_note_show(int argc, char* argv[]) {
    if(argc < 1) {
        log_error("Missing note ID.\n");
        return R_ERROR;
    }

    struct note_list list;
    note_list_init(&list);

    if(read_notes("NOTES.md", &list) != R_OK) {
        log_critical("Failed to read NOTES.md\n");
        note_list_free(&list);
        return R_ERROR;
    }

    struct note *note = note_list_find_by_id(&list, argv[0]);
    if(note == NULL) {
        log_error("Note not found.");
        note_list_free(&list);
        return R_ERROR;
    }

    printf("🗒️ Note %s\n", (note->id[0] == '\0' ? "<missing id>" : note->id));
    print_divider(60);

    char date_buffer[DEFAULT_BUFFER_SIZE];
    format_date(note->created, date_buffer, sizeof(date_buffer));
    printf(COLOR_METADATA"%-15s: %s\n" ANSI_RESET, "Created:", date_buffer);

    printf(COLOR_METADATA "%-15s: %s\n" ANSI_RESET, "Topic:", note->topic);

    struct tag_list tags;
    extract_note_tags(note, &tags);
    printf(COLOR_METADATA "%-15s: ", "Tags:");
    for(size_t i = 0; i < tags.count; ++i) {
        printf("#%s ", tags.items[i]);
    }
    printf(ANSI_RESET "\n\n");

    printf("%s\n", note->text);

    print_divider(60);

    return R_ERROR;
}

int command_note_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    size_t command_count = 0;
    const struct command_definition *commands = get_note_commands(&command_count);

    return print_subcommand_help("note", "allows you to inspect and manage stored notes.", commands, command_count);
}

static const struct command_definition note_commands[] = {
    {
        "help",
        "",
        "display help to the `note` command",
        command_note_help,
        NULL,
        0,
        true
    },
    {
        "show",
        "<id>",
        "Show the details of a note",
        command_note_show,
        NULL,
        0,
        true
    }
};

const struct command_definition* get_note_commands(size_t* count) {
    if(count != NULL) {
        *count = sizeof(note_commands) / sizeof(note_commands[0]);
    }

    return note_commands;
}
