#include <string.h>
#include <stdio.h>
#include "common.h"
#include "cli.h"
#include "logging.h"
#include "commands.h"
#include "note.h"

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
        "        'note',\n"
        "        'tag',\n"
        "        'today',\n"
        "        'todo',\n"
        "        'topic',\n"
        "        'util',\n"
        "        'version'\n"
        "    )\n"
        "\n"
        "    $noteCommands = @(\n"
        "        'show',\n"
        "        'edit',\n"
        "        'retopic',\n"
        "        'remove',\n"
        "        'help'\n"
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
        "    elseif ($elements[1].Extent.Text -eq 'note' -and $elements.Count -le 3) {\n"
        "        $candidates = $noteCommands\n"
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

int command_util(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    log_error("Please specify a util command. Refer to `util help` if required.");
 
    return R_ERROR;
}

static int migrate_notes_v0_to_v1(void) {
    struct note_list all_notes;
    note_list_init(&all_notes);

    if(read_notes(NOTES_FILE, &all_notes) != R_OK) {
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
    if(read_notes_metadata(NOTES_FILE, &metadata) != R_OK) {
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

static int command_util_help(int argc, char* argv[]);

static const struct command_definition util_commands[] = {
    {
        "help",
        "",
        "Display help about the available commands",
        command_util_help,
        NULL,
        0,
        false
    },
    {
        "completion",
        "<shell>",
        "Generate shell completion definitions",
        command_util_completion,
        NULL,
        0,
        false
    },
    {
        "migrate",
        "",
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

static int command_util_help(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    size_t command_count = 0;
    const struct command_definition *commands = get_util_commands(&command_count);
    return print_subcommand_help("util", "povides utility and integration commands.", commands, command_count);
}
