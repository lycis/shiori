#include <string.h>
#include <stdio.h>
#include "common.h"
#include "cli.h"
#include "logging.h"
#include "commands.h"
#include "note.h"
#include "cmd_shared.h"
#include "platform.h"

static int print_powershell_completion(void)
{
    fputs(
        "Register-ArgumentCompleter -Native -CommandName shiori -ScriptBlock {\n"
        "    param($wordToComplete, $commandAst, $cursorPosition)\n"
        "\n"
        "    $elements = @($commandAst.CommandElements)\n"
        "\n"
        "    $topLevel = @(\n"
        "        'add',\n"
        "        'capture',\n"
        "        'config',\n"
        "        'console',\n"
        "        'tag',\n"
        "        'today',\n"
        "        'todo',\n"
        "        'topic',\n"
        "        'util',\n"
        "        'version'\n"
        "    )\n"
        "\n"
        "    $todoCommands = @(\n"
        "        'add',\n"
        "        'list',\n"
        "        'start',\n"
        "        'done',\n"
        "        'reopen',\n"
        "        'rewrite',\n"
        "        'remove',\n"
        "        'prune'\n"
        "    )\n"
        "\n"
        "    $utilCommands = @(\n"
        "        'completion'\n"
        "    )\n"
        "\n"
        "    $shells = @(\n"
        "        'powershell'\n"
        "    )\n"
        "\n"
        "    $candidates = @()\n"
        "\n"
        "    if ($elements.Count -le 2) {\n"
        "        $candidates = $topLevel\n"
        "    }\n"
        "    elseif ($elements[1].Extent.Text -eq 'todo' -and $elements.Count -le 3) {\n"
        "        $candidates = $todoCommands\n"
        "    }\n"
        "    elseif ($elements[1].Extent.Text -eq 'util') {\n"
        "        if ($elements.Count -le 3) {\n"
        "            $candidates = $utilCommands\n"
        "        }\n"
        "        elseif ($elements[2].Extent.Text -eq 'completion' -and $elements.Count -le 4) {\n"
        "            $candidates = $shells\n"
        "        }\n"
        "    }\n"
        "\n"
        "    $candidates |\n"
        "        Where-Object { $_ -like \"$wordToComplete*\" } |\n"
        "        ForEach-Object {\n"
        "            [System.Management.Automation.CompletionResult]::new(\n"
        "                $_,\n"
        "                $_,\n"
        "                'ParameterValue',\n"
        "                $_\n"
        "            )\n"
        "        }\n"
        "}\n",
        stdout
    );

    return R_OK;
}

int command_util_completion(int argc, char *argv[]) {
    if(has_switch(argc, argv, "-h", false) || has_switch(argc, argv, "--help", false)) {
        printf(
            "Usage:\n"
            "  %s util completion <shell>\n"
            "\n"
            "Generates shell completion definitions for Shiori.\n"
            "The generated script is written to standard output.\n"
            "\n"
            "Supported shells:\n"
            "  %-22s PowerShell completion script\n"
            "\n"
            "Examples:\n"
            "  %s util completion powershell\n"
            "  %s util completion powershell > shiori-completion.ps1\n"
            "  %s util completion powershell  | Out-String | Invoke-Expression\n",
            APP_NAME,
            "powershell",
            APP_NAME,
            APP_NAME, 
            APP_NAME
        );
        return R_OK;
    }

    if(argc == 0) {
        log_error("Missing shell.\n");
        return R_ERROR;
    }

    if(strcmp(argv[0], "powershell") == 0 || strcmp(argv[0], "pwsh") == 0) {
        return print_powershell_completion();
    }

   /* if(strcmp(argv[0], "bash") == 0 || strcmp(argv[0], "sh") == 0) {
        return print_bash_completion();
    }

    if(strcmp(argv[0], "zsh") == 0) {
        return print_zsh_completion();
    } */

    log_error("Unsupported shell: %s\n", argv[0]);
    return R_ERROR;
}

static void print_util_help(void)
{
    printf(
        "Usage:\n"
        "  %s util <command> [options]\n"
        "\n"
        "Utility commands for Shiori maintenance and integrations.\n"
        "\n"
        "Commands:\n"
        "  %-22s Generate shell completion definitions\n"
        "\n"
        "Run `%s util <command> --help` for more information about a command.\n"
        "\n"
        "Examples:\n"
        "  %s util completion powershell\n",
        APP_NAME,
        "completion <shell>",
        APP_NAME,
        APP_NAME
    );
}

int command_util(int argc, char *argv[]) {
    if(argc == 0 || has_switch(argc, argv, "--help", true)) {
        print_util_help();
        return R_OK;
    }

    return R_ERROR;
}

static int rewrite_notes(struct note_list *notes, struct notes_metadata *md) {
    if(notes == NULL || md == NULL) {
        return R_ERROR;
    }

    char file_path[DEFAULT_BUFFER_SIZE];
    if(get_base_dir_file_path("NOTES.md", file_path, sizeof(file_path)) != R_OK) {
        return R_ERROR;
    }

    char temp_file[DEFAULT_BUFFER_SIZE];
    int written = snprintf(temp_file, sizeof(temp_file), "%s.tmp", "NOTES.md");
    if(written < 0 || (size_t)written >= sizeof(temp_file)) {
        log_error("Temporary file path is too long.\n");
        return R_ERROR;
    }

    char temp_path[DEFAULT_BUFFER_SIZE];
    if(get_base_dir_file_path(temp_file, temp_path, sizeof(temp_path)) != R_OK) {
        return R_ERROR;
    }

    char backup_path[DEFAULT_BUFFER_SIZE];
    written = snprintf(backup_path, sizeof(backup_path), "%s.bak", file_path);
    if(written < 0 || (size_t)written >= sizeof(backup_path)) {
        log_error("Backup file path is too long.\n");
        return R_ERROR;
    }

    FILE *temp = open_base_dir_file(temp_file, "w");
    if(temp == NULL) {
        log_error("Failed opening temporary notes file.\n");
        return R_ERROR;
    }

    fprintf(temp,
        "---\n"
        "version: %d\n"
        "---\n",
        md->version
    );

    time_t last_date = 0;

    for(size_t i = 0; i < notes->count; ++i) {
        struct note *note = &notes->items[i];

        if(i == 0 || !dates_equal(note->created, last_date)) {
            char heading[DEFAULT_BUFFER_SIZE];

            if(build_daily_heading(heading, sizeof(heading), note->created) != R_OK) {
                fclose(temp);
                remove(temp_path);
                return R_ERROR;
            }

            fprintf(temp, "\n%s\n", heading);
            last_date = note->created;
        }

        if(write_note(temp, note) != R_OK) {
            fclose(temp);
            remove(temp_path);
            log_error("Failed writing note to temporary file.\n");
            return R_ERROR;
        }
    }

    if(fclose(temp) != 0) {
        remove(temp_path);
        log_error("Failed closing temporary notes file.\n");
        return R_ERROR;
    }

    /*
     * Remove an old backup if one is still around.
     */
    if(access(backup_path, F_OK) == 0) {
        if(remove(backup_path) != 0) {
            remove(temp_path);
            log_error("Failed removing previous NOTES.md backup.\n");
            return R_ERROR;
        }
    }

    /*
     * Move the existing file out of the way.
     */
    if(rename(file_path, backup_path) != 0) {
        remove(temp_path);
        log_error("Failed creating NOTES.md backup.\n");
        return R_ERROR;
    }

    /*
     * Put the newly written file in place.
     */
    if(rename(temp_path, file_path) != 0) {
        log_error("Failed replacing NOTES.md.\n");

        if(rename(backup_path, file_path) != 0) {
            log_critical("Failed restoring NOTES.md from backup.\n");
        }

        remove(temp_path);
        return R_ERROR;
    }

    /*
     * New file is safely in place, backup is no longer needed.
     */
    if(remove(backup_path) != 0) {
        log_warning("Failed removing NOTES.md backup.\n");
    }

    return R_OK;
}

static int migrate_notes_v0_to_v1(void) {
    struct note_list all_notes;
    note_list_init(&all_notes);

    if(read_notes("NOTES.md", &all_notes) != R_OK) {
        log_critical("Failed to read all notes.\n");
        note_list_free(&all_notes);
        return R_ERROR;
    }

    size_t migrated_count = 0;
    time_t last_date = 0;
    unsigned int seqnr = 0;

    for(size_t i = 0; i < all_notes.count; ++i) {
        struct note *n = &all_notes.items[i];

        if(i == 0 || !dates_equal(n->created, last_date)) {
            last_date = n->created;
            seqnr = 1;
        }

        // preserve existing ID
        if(n->id[0] != '\0') {
            const char *dash = strrchr(n->id, '-');

            if(dash != NULL) {
                unsigned int existing_seq = 0;

                if(sscanf_s(dash + 1, "%u", &existing_seq) == 1 &&
                   existing_seq >= seqnr) {
                    seqnr = existing_seq + 1;
                }
            }

            continue;
        }

        char date_buffer[16];

        if(format_date(n->created, date_buffer, sizeof(date_buffer)) != R_OK) {
            note_list_free(&all_notes);
            log_critical("Failed to format note date.\n");
            return R_ERROR;
        }

        char id_date[9];
        int written = snprintf(id_date, sizeof(id_date), "%.4s%.2s%.2s", date_buffer, date_buffer + 5, date_buffer + 8);
        if(written < 0 || (size_t)written >= sizeof(id_date)) {
            note_list_free(&all_notes);
            log_critical("Failed to generate note id date.\n");
            return R_ERROR;
        }

        written = snprintf(n->id, sizeof(n->id), "%s-%04u", id_date, seqnr++);
        if(written < 0 || (size_t)written >= sizeof(n->id)) {
            note_list_free(&all_notes);
            log_error("Generated note ID is too long.\n");
            return R_ERROR;
        }

        migrated_count++;
    }

    struct notes_metadata md;
    md.version = 1;

    if(rewrite_notes(&all_notes, &md) != R_OK) {
        note_list_free(&all_notes);
        log_critical("Failed to rewrite NOTES.md.\n");
        return R_ERROR;
    }

    note_list_free(&all_notes);

    log_success("Added IDs to %zu note%s.\n",
        migrated_count,
        migrated_count == 1 ? "" : "s");

    log_success("Migrated NOTES.md from version 0 to 1.\n");

    return R_OK;
}

static int migrate_notes() {
    struct notes_metadata metadata;
    if(read_notes_metadata("NOTES.md", &metadata) != R_OK) {
        log_critical("Failed to read notes metadata.\n");
        return R_ERROR;
    }

    while(metadata.version < NOTES_FORMAT_VERSION) {
        switch(metadata.version) {
            case 0:
                if(migrate_notes_v0_to_v1() != R_OK) {
                    return R_ERROR;
                }

                metadata.version = 1;
                break;

            default:
                log_error(
                    "No migration path for notes version %d.\n",
                    metadata.version
                );
                return R_ERROR;
        }
    }

    return R_OK;
}

static int command_util_migrate(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if(migrate_notes() != R_OK) {
        log_critical("Migrating NOTES.md to current version failed. Your workspace may be broken!\n");
        return R_ERROR;
    }
    return R_OK;
}

static const struct command_definition util_commands[] = {
    {
        "completion",
        "Generate shell completion definitions",
        command_util_completion,
        NULL,
        0,
        false
    },
    {
        "migrate",
        "Run necessary migrations to bring your workspace to the latest version",
        command_util_migrate,
        NULL,
        0,
        false
    }
};


const struct command_definition* get_util_commands(size_t* count) {
    if(count != NULL) {
        *count = sizeof(util_commands) / sizeof(util_commands[0]);
    }

    return util_commands;
}
