#include "common.h"
#include "config.h"
#include "logging.h"
#include "platform.h"

int hook_after_command(const char *command, int argc, char *argv[]) {
    log_debug("calling after_command hook");

    set_environment_variable("SHIORI_VERSION", APP_VERSION);
    set_environment_variable("SHIORI_COMMAND", command);

    char args[DEFAULT_BUFFER_SIZE];
    if(join_array(argc, argv, args, sizeof(args)) != R_OK) {
        log_warning("Executing hook_after_command failed: Problem with arg passing.\n");
        return R_ERROR;
    }
    set_environment_variable("SHIORI_COMMAND_ARGS", args);

    int exit_code = run_script_basedir(g_config.hooks.after_command);
    if(exit_code != 0) {
        log_warning("Executing hook_after_command failed: Exit code %d.", exit_code);
    }

    return R_OK;
}