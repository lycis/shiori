#include <time.h>
#include "common.h"
#include "logging.h"
#include "cmd_shared.h"

int command_add(int argc, char* argv[]) {
    log_debug("Adding new note.\n");
    
    char heading[32];
    if(build_daily_heading(heading, sizeof(heading), time(NULL)) != R_OK) {
        return R_ERROR;
    }

    if(add_markdown_item(argc, argv, "NOTES.md", "* ", heading) != R_OK) {
        return R_ERROR;
    }

    log_success("Added your note.\n");
    return R_OK;
}