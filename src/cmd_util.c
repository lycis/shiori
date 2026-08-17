#include <string.h>
#include <stdio.h>
#include "common.h"
#include "cli.h"
#include "logging.h"
#include "commands.h"

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

static const struct command_definition util_commands[] = {
    {
        "completion",
        "Generate shell completion definitions",
        command_util_completion,
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
