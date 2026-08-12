#include <time.h>
#include <stdio.h>
#include "common.h"
#include "logging.h"
#include "cmd_shared.h"

int build_daily_heading(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm local_time;
    if(localtime_s(&local_time, &now) != 0) {
        log_error("Failed to get local time.");
        return R_ERROR;
    }

    char date_str[11];
    if(strftime(date_str, sizeof(date_str), "%Y-%m-%d", &local_time) == 0) {
        log_error("Feild to format local date.");        
        return R_ERROR;
    }

    int written = snprintf(buffer, size, "# %s", date_str);
    if(written < 0 || written >= (int)size) {
        log_critical("Daily heading buffer too small.");
        return R_ERROR;
    }

    return R_OK;
}
int command_add(int argc, char* argv[]) {
    log_debug("Adding new note.\n");
    
    char heading[32];
    if(build_daily_heading(heading, sizeof(heading)) != R_OK) {
        return R_ERROR;
    }

    if(add_markdown_item(argc, argv, "NOTES.md", "* ", heading) != R_OK) {
        return R_ERROR;
    }

    log_success("Added your note.\n");
    return R_OK;
}