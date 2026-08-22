#include <string.h>
#include <time.h>

#include "cli.h"
#include "cmd_shared.h"
#include "color.h"
#include "common.h"
#include "logging.h"
#include "note.h"

static int build_note_from_args(int argc, char *argv[], struct note *note) {
    if(argc <= 0 || argv == NULL || note == NULL) {
        return R_ERROR;
    }

    char *text_argv[argc];
    int text_argc = 0;

    const char *topic = NULL;

    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--topic") == 0) {
            if(i + 1 >= argc) {
                log_error("%s requires a topic name.\n", argv[i]);
                return R_ERROR;
            }

            topic = argv[++i];
            continue;
        }

        text_argv[text_argc++] = argv[i];
    }

    if(text_argc == 0) {
        log_error("Note text cannot be empty.\n");
        return R_ERROR;
    }

    char buffer[DEFAULT_BUFFER_SIZE * 3];

    if(join_array(text_argc, text_argv, buffer, sizeof(buffer)) != R_OK) {
        return R_ERROR;
    }

    if(create_note_from_markdown(buffer, time(NULL), note) != R_OK) {
        log_critical("Error creating note.\n");
        return R_ERROR;
    }

    if(topic != NULL) {
        if(strcpy_s(note->topic, sizeof(note->topic), topic) != 0) {
            log_error("Topic name is too long.\n");
            return R_ERROR;
        }
    }

    return R_OK;
}

static int add_note_to_markdown(struct note *note, const char *filename, const char *heading) {
    if(note == NULL) {
        return R_ERROR;
    }

    char topic_buffer[DEFAULT_BUFFER_SIZE];
    char id_buffer[DEFAULT_BUFFER_SIZE];

    char *argv[3];
    int argc = 0;

    argv[argc++] = note->text;

    if(note->topic[0] != '\0') {
        int written = snprintf(topic_buffer, sizeof(topic_buffer), "#%s/topic/%s", APP_NAME, note->topic);
        if(written < 0 || (size_t)written >= sizeof(topic_buffer)) {
            log_error("Topic title is too long.\n");
            return R_ERROR;
        }

        argv[argc++] = topic_buffer;
    }

    if(note->id[0] != '\0') {
        int written = snprintf(id_buffer, sizeof(id_buffer), "<!-- %s:id=%s -->", APP_NAME, note->id);
        if(written < 0 || (size_t)written >= sizeof(id_buffer)) {
            log_error("Note ID is too long.\n");
            return R_ERROR;
        }

        argv[argc++] = id_buffer;
    }

    return add_markdown_item(argc, argv, filename, "* ", heading);
}

static int next_note_id_sequence_for(const struct note_list *notes, unsigned int *result) {
    if(notes == NULL || result == NULL) {
        return R_ERROR;
    }

    unsigned int max_sequence = 0;

    for(size_t i = 0; i < notes->count; ++i) {
        const char *id = notes->items[i].id;

        if(id[0] == '\0') {
            continue;
        }

        unsigned int sequence = 0;

        const char *dash = strrchr(id, '-');

        if(dash == NULL) {
            log_warning("Ignoring note with invalid id '%s'.\n", id);
            continue;
        }

        if(sscanf_s(dash + 1, "%u", &sequence) != 1) {
            log_warning("Ignoring note with invalid id '%s'.\n", id);
            continue;
        }

        if(sequence > max_sequence) {
            max_sequence = sequence;
        }
    }

    *result = max_sequence + 1;

    return R_OK;
}

static int assign_note_id(struct note *note) {
    if(note == NULL) {
        return R_ERROR;
    }

    struct note_list notes;
    note_list_init(&notes);

    if(read_notes_for_date(NOTES_FILE, note->created, &notes) != R_OK) {
        log_critical("Failed reading notes for date.\n");
        note_list_free(&notes);
        return R_ERROR;
    }

    char date_buffer[16];

    if(format_date(note->created, date_buffer, sizeof(date_buffer)) != R_OK) {
        note_list_free(&notes);
        return R_ERROR;
    }

    char id_date[9];

    int written = snprintf(id_date, sizeof(id_date), "%.4s%.2s%.2s", date_buffer, date_buffer + 5, date_buffer + 8);

    if(written < 0 || (size_t)written >= sizeof(id_date)) {
        note_list_free(&notes);
        return R_ERROR;
    }

    unsigned int sequence = 0;

    if(next_note_id_sequence_for(&notes, &sequence) != R_OK) {
        note_list_free(&notes);
        return R_ERROR;
    }

    note_list_free(&notes);

    written = snprintf(note->id, sizeof(note->id), "%s-%04u", id_date, sequence);

    if(written < 0 || (size_t)written >= sizeof(note->id)) {
        log_error("Generated note ID is too long.\n");
        return R_ERROR;
    }

    return R_OK;
}

int command_add(int argc, char *argv[]) {
    if(has_switch(argc, argv, "--help", false) || has_switch(argc, argv, "-h", false)) {
        printf(
            "Usage:\n"
            "  %s add [options] <text...>\n"
            "\n"
            "Adds a note to today's daily section.\n"
            "\n"
            "Options:\n"
            "  %-22s Assign a topic to the note\n"
            "  %-22s Show this help\n"
            "\n"
            "Examples:\n"
            "  %s add Remember to review the proposal\n"
            "  %s add -t Rail4Climate Discuss pilot scope\n",
            APP_NAME,
            "-t, --topic <topic>",
            "-h, --help",
            APP_NAME,
            APP_NAME
        );

        return R_OK;
    }

    log_debug("Adding new note.\n");

    struct note n;
    if(build_note_from_args(argc, argv, &n) != R_OK) {
        log_critical("Failed to build note from input text.\n");
        return R_ERROR;
    }

    struct note_list notes_before;
    note_list_init(&notes_before);
    if(read_notes(NOTES_FILE, &notes_before) != R_OK) {
        note_list_free(&notes_before);
        return R_ERROR;
    }
    size_t previous_count = notes_before.count;
    note_list_free(&notes_before);

    if(assign_note_id(&n) != R_OK) {
        log_critical("Coult not assign note id.\n");
        return R_ERROR;
    }

    char heading[DEFAULT_BUFFER_SIZE];

    if(build_daily_heading(heading, sizeof(heading), n.created) != R_OK) {
        return R_ERROR;
    }

    if(add_note_to_markdown(&n, NOTES_FILE, heading) != R_OK) {
        return R_ERROR;
    }

    struct note_list notes_after;
    note_list_init(&notes_after);
    int validation_result = read_notes(NOTES_FILE, &notes_after);
    size_t new_count = notes_after.count;
    note_list_free(&notes_after);

    if(validation_result != R_OK || new_count != previous_count + 1) {
        log_critical(
            "NOTES.md post-write validation failed: expected %zu notes, found %zu.\n",
            previous_count + 1,
            new_count
        );
        restore_notes_backup();
        return R_ERROR;
    }

    if(n.topic[0] == '\0') {
        log_success("Added note " COLOR_NOTE_ID "%s" ANSI_RESET ".\n", n.id);
    } else {
        log_success(
            "Added note " COLOR_NOTE_ID "%s" ANSI_RESET " %s[%s]%s\n",
            n.id,
            COLOR_SUCCESS,
            n.topic,
            ANSI_RESET
        );
    }
    return R_OK;
}
