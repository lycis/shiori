#include <time.h>
#include <string.h>
#include "cli.h"
#include "common.h"
#include "logging.h"
#include "cmd_shared.h"

int command_add(int argc, char* argv[]) {
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

    const char *topic = NULL;

    char *note_argv[argc + 1];
    int note_argc = 0;

    // let's build our note string
    for(int i = 0; i < argc; ++i) {
        if(strcmp(argv[i], "--topic") == 0 || strcmp(argv[i], "-t") == 0) {
            if(i + 1 >= argc) {
                log_error("--topic requires a topic name.\n");
                return R_ERROR;
            }

            topic = argv[i + 1];
            i++;
            continue;
        }

        note_argv[note_argc++] = argv[i];
    }

    char topic_tag[DEFAULT_BUFFER_SIZE];
    if(topic != NULL) {
        log_debug("Identified topic '%s' for the note.\n", topic);

        int written = snprintf(topic_tag, sizeof(topic_tag), "#%s/topic/%s", APP_NAME, topic);
        if(written < 0 || (size_t)written >= sizeof(topic_tag)) {
            log_error("Topic title is too long.\n");
            return R_ERROR;
        }

        // add generated tag at the end
        note_argv[note_argc++] = topic_tag;
    }

    if(note_argc == 0) {
        log_error("You must provide a note text.\n");
        return R_ERROR;
    }
    
    char heading[32];
    if(build_daily_heading(heading, sizeof(heading), time(NULL)) != R_OK) {
        return R_ERROR;
    }

    if(add_markdown_item(note_argc, note_argv, "NOTES.md", "* ", heading) != R_OK) {
        return R_ERROR;
    }

    if(topic == NULL) log_success("Added your note.\n");
    else log_success("Added your note. %s[%s]%s\n", ANSI_FG_RGB(140, 180, 255), topic, ANSI_RESET);
    return R_OK;
}