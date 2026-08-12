#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "logging.h"
#include "cli.h"
#include "platform.h"
#include "common.h"

int command_init(int argc, char* argv[]) {
    bool config_exists = access(CONFIG_FILE_NAME, F_OK) == 0;

    // check if there is already a config
    if(!has_switch(argc, argv, "--reinit")) {
        if(config_exists) {
            log_error("%s already exists in current directory\n", CONFIG_FILE_NAME);
            printf("If you want to reinitialize, please delete the existing %s file first or specify --reinit.\n", CONFIG_FILE_NAME);
            return R_ERROR;
        }
    }

    if(config_exists) {
        log_info("Reinitializing config in current directory.\n");
    }

    FILE *config_file = NULL;
    errno_t err = fopen_s(&config_file, CONFIG_FILE_NAME, "w");
    if(err != 0 || config_file == NULL) {
        log_error("Error creating config file in current directory\n");
        return R_ERROR;
    }

    // write comment with generation date to file
    time_t now = time(NULL);
    char date_str[26]; // ctime_s requires a buffer of at least 26 bytes
    ctime_s(date_str, 26, &now);
    fprintf(config_file, "# Initialized: %s", date_str);

    // write up to date config file version
    fprintf(config_file, "version: %d\n", CONFIG_VERSION);

    // by default the base dir for keeping notes is the current work dir
    char buffer[DEFAULT_BUFFER_SIZE];
    get_current_path(buffer, DEFAULT_BUFFER_SIZE);
    fprintf(config_file, "base_dir: %s\n", buffer);

    fclose(config_file);

    log_success("%s config file created in current directory\n", CONFIG_FILE_NAME);
    return 0;
}
