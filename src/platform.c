#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "common.h"
#include "platform.h"
#include "logging.h"
#include "config.h"
#include "color.h"

#ifdef _WIN32
static wchar_t *utf8_path_to_wide(const char *value) {
    if(value == NULL) {
        return NULL;
    }

    int required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value,
        -1,
        NULL,
        0
    );

    if(required <= 0) {
        return NULL;
    }

    wchar_t *wide = malloc((size_t)required * sizeof(wchar_t));
    if(wide == NULL) {
        return NULL;
    }

    if(MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value,
        -1,
        wide,
        required
    ) <= 0) {
        free(wide);
        return NULL;
    }

    return wide;
}
#endif

int file_access_utf8(const char *path, int mode) {
#ifdef _WIN32
    wchar_t *wide_path = utf8_path_to_wide(path);
    if(wide_path == NULL) {
        return -1;
    }

    int result = _waccess(wide_path, mode);
    free(wide_path);
    return result;
#else
    return access(path, mode);
#endif
}

int file_open_utf8(FILE **file, const char *path, const char *mode) {
    if(file == NULL) {
        return EINVAL;
    }

    *file = NULL;

#ifdef _WIN32
    wchar_t *wide_path = utf8_path_to_wide(path);
    wchar_t *wide_mode = utf8_path_to_wide(mode);

    if(wide_path == NULL || wide_mode == NULL) {
        free(wide_path);
        free(wide_mode);
        return EINVAL;
    }

    errno_t result = _wfopen_s(file, wide_path, wide_mode);
    free(wide_path);
    free(wide_mode);
    return result;
#else
    *file = fopen(path, mode);
    return *file == NULL ? errno : 0;
#endif
}

int file_remove_utf8(const char *path) {
#ifdef _WIN32
    wchar_t *wide_path = utf8_path_to_wide(path);
    if(wide_path == NULL) {
        return -1;
    }

    int result = _wremove(wide_path);
    free(wide_path);
    return result;
#else
    return remove(path);
#endif
}

int file_rename_utf8(const char *old_path, const char *new_path) {
#ifdef _WIN32
    wchar_t *wide_old_path = utf8_path_to_wide(old_path);
    wchar_t *wide_new_path = utf8_path_to_wide(new_path);

    if(wide_old_path == NULL || wide_new_path == NULL) {
        free(wide_old_path);
        free(wide_new_path);
        return -1;
    }

    int result = _wrename(wide_old_path, wide_new_path);
    free(wide_old_path);
    free(wide_new_path);
    return result;
#else
    return rename(old_path, new_path);
#endif
}

char* get_path_separator() {
    #ifdef _WIN32
    return "\\";
    #else
    return "/";
    #endif
}

int terminal_enable_utf8(void) {
    #ifdef _WIN32
    if(GetConsoleOutputCP() == CP_UTF8) {
        return R_OK;
    }

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    return R_OK;
    #else
     setlocale(LC_CTYPE, "");

    const char *encoding = nl_langinfo(CODESET);

    bool is_utf8 = strcmp(encoding, "UTF-8") == 0 || strcmp(encoding, "UTF8") == 0;
    return is_utf8 ? R_OK : R_ERROR;
    #endif
}

int get_user_home(char* buffer, size_t size) {
    #ifdef _WIN32
    size_t required = 0;

    if (getenv_s(&required, buffer, size, "USERPROFILE") != 0 ||
        required == 0) {
        return R_ERROR;
    }

    return R_OK;
    #else
    #error "get_user_home is only implemented for Windows"
    #endif
}

char* get_current_path(char *buffer, size_t size) {
    #ifdef _WIN32
    char* ptr = _getcwd(buffer, (int) size);
    #else
    char* ptr =  getcwd(buffer, size);
    #endif

    return ptr;
}

int set_environment_variable(const char* name, const char *value) {
    #ifdef _WIN32
    errno_t err = _putenv_s(name, value);
    if(err != 0) return R_ERROR;
    return R_OK;
    #else
    #error not implemented
    #endif
}

int run_process(const char *path, const char* working_dir) {
#ifdef _WIN32
    STARTUPINFOA startup_info = {0};
    PROCESS_INFORMATION process_info = {0};

    startup_info.cb = sizeof(startup_info);

    char command_line[DEFAULT_BUFFER_SIZE * 2];
    int written = snprintf(command_line, sizeof(command_line), "%s", path);
    if(written < 0 || (size_t)written >= sizeof(command_line)) {
        log_critical("Process command line is too long");
        return -1;
    }

    BOOL success = CreateProcessA(
        NULL,
        command_line,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        working_dir,
        &startup_info,
        &process_info
    );

    if(!success) {
         log_error("Failed starting process '%s' (error %lu).\n", path, GetLastError());
        return -1;
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);

    DWORD exit_code;
    if(!GetExitCodeProcess(process_info.hProcess, &exit_code)) {
        log_error("Failed reading process exit code (error %lu)\n", GetLastError());
        CloseHandle(process_info.hThread);
        CloseHandle(process_info.hProcess);
        return -1;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    return (int)exit_code;
#else
#error not implemented
#endif
}

int run_script_basedir(const char* path) {
    if(path == NULL || path[0] == '\0') {
        return -1;
    }

    char script_path[DEFAULT_BUFFER_SIZE * 2];

    if(get_base_dir_file_path(path, script_path, sizeof(script_path)) != R_OK) {
        log_error("Script path is too long.\n");
        return -1;
    }

#ifdef _WIN32
    if(str_ends_with(script_path, ".bat") || str_ends_with(script_path, ".cmd")) {
        char command[DEFAULT_BUFFER_SIZE * 3];
        int written = snprintf(command, sizeof(command), "cmd.exe /c \"\"%s\"\"", script_path);
        if(written < 0 || (size_t)written >= sizeof(command)) {
            log_error("Script command (%s) is too long.\n", path);
            return -1;
        }

        return run_process(command, g_config.base_dir);
    }
#else
#error not implemented
#endif

    return run_process(script_path, g_config.base_dir);
}

// Terminal rendering functions
#ifdef _WIN32
static DWORD g_original_console_input_mode = 0;
static DWORD g_original_console_output_mode = 0;

static bool g_interactive_mode_active = false;

static HANDLE g_console_input = NULL;
static HANDLE g_console_output = NULL;

static size_t g_previous_suggestion_lines = 0;

int terminal_enter_interactive_mode(void)
{
    g_console_input = GetStdHandle(STD_INPUT_HANDLE);
    g_console_output = GetStdHandle(STD_OUTPUT_HANDLE);

    if(g_console_input == INVALID_HANDLE_VALUE ||
       g_console_input == NULL) {
        log_error("Failed getting console input handle.\n");
        return R_ERROR;
    }

    if(g_console_output == INVALID_HANDLE_VALUE ||
       g_console_output == NULL) {
        log_error("Failed getting console output handle.\n");
        return R_ERROR;
    }

    /*
     * Input mode
     */
    DWORD input_mode;

    if(!GetConsoleMode(g_console_input, &input_mode)) {
        log_error(
            "Failed reading console input mode (error %lu).\n",
            GetLastError()
        );
        return R_ERROR;
    }

    g_original_console_input_mode = input_mode;

    input_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    input_mode |= ENABLE_PROCESSED_INPUT;

    if(!SetConsoleMode(g_console_input, input_mode)) {
        log_error(
            "Failed enabling interactive input mode (error %lu).\n",
            GetLastError()
        );
        return R_ERROR;
    }

    /*
     * Output mode
     */
    DWORD output_mode;

    if(!GetConsoleMode(g_console_output, &output_mode)) {
        log_error(
            "Failed reading console output mode (error %lu).\n",
            GetLastError()
        );

        SetConsoleMode(
            g_console_input,
            g_original_console_input_mode
        );

        return R_ERROR;
    }

    g_original_console_output_mode = output_mode;

    output_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    if(!SetConsoleMode(g_console_output, output_mode)) {
        log_error(
            "Failed enabling virtual terminal processing (error %lu).\n",
            GetLastError()
        );

        SetConsoleMode(
            g_console_input,
            g_original_console_input_mode
        );

        return R_ERROR;
    }

    g_interactive_mode_active = true;

    return R_OK;
}

void terminal_leave_interactive_mode(void)
{
    if(!g_interactive_mode_active) {
        return;
    }

    if(!SetConsoleMode(
        g_console_input,
        g_original_console_input_mode
    )) {
        log_warning(
            "Failed restoring console input mode (error %lu).\n",
            GetLastError()
        );
    }

    if(!SetConsoleMode(
        g_console_output,
        g_original_console_output_mode
    )) {
        log_warning(
            "Failed restoring console output mode (error %lu).\n",
            GetLastError()
        );
    }

    g_interactive_mode_active = false;
}

void terminal_finish_input_line(void)
{
    /*
     * We are currently on the input line.
     * Clear all suggestion lines below it.
     */
    for(size_t i = 0; i < g_previous_suggestion_lines; ++i) {
        printf("\n\x1b[2K");
    }

    /*
     * Leave the cursor one line below the submitted input,
     * not at the bottom of the old suggestion block.
     */
    if(g_previous_suggestion_lines > 0) {
        printf(
            "\x1b[%zuA",
            g_previous_suggestion_lines
        );
    }

    printf("\n");

    g_previous_suggestion_lines = 0;

    fflush(stdout);
}
static const char *current_token(const char *buffer)
{
    if(buffer == NULL) {
        return "";
    }

    const char *token = buffer;

    for(const char *p = buffer; *p != '\0'; ++p) {
        if(*p == ' ' || *p == '\t') {
            token = p + 1;
        }
    }

    return token;
}

void terminal_render_input(
    const char *prompt,
    const char *buffer,
    size_t cursor,
    const struct completion_result *completions
) {
    /*
     * Clear the current input line and all suggestion lines
     * left behind by the previous render.
     */
    printf("\r\x1b[2K");

    for(size_t i = 0; i < g_previous_suggestion_lines; ++i) {
        printf("\n\x1b[2K");
    }

    /*
     * Return to the input line after clearing old suggestions.
     */
    if(g_previous_suggestion_lines > 0) {
        printf(
            "\x1b[%zuA\r",
            g_previous_suggestion_lines
        );
    }

    /*
     * Draw the complete current input.
     */
    printf("%s%s", prompt, buffer);

    /*
     * Completion applies to the current token only.
     *
     * Example:
     *
     *   buffer:     "todo ad"
     *   token:      "ad"
     *   suggestion: "add"
     */
    const char *token = current_token(buffer);
    size_t typed_length = strlen(token);

    size_t rendered_suggestions = 0;

    if(completions != NULL) {
        for(size_t i = 0; i < completions->count; ++i) {
            const char *suggestion = completions->items[i];

            if(suggestion == NULL) {
                continue;
            }

            /*
             * Don't show an exact match as a suggestion.
             */
            if(strcmp(suggestion, token) == 0) {
                continue;
            }

            printf("\n\x1b[2K  ");

            /*
             * Already typed portion in green.
             */
            printf(
                "%s%.*s%s",
                COLOR_COMPLETION_MATCH,
                (int)typed_length,
                suggestion,
                ANSI_RESET
            );

            /*
             * Remaining portion in grey.
             */
            printf(
                "%s%s%s",
                COLOR_COMPLETION_REMAINDER,
                suggestion + typed_length,
                ANSI_RESET
            );

            rendered_suggestions++;
        }
    }

    /*
     * Return to the input line.
     */
    if(rendered_suggestions > 0) {
        printf(
            "\x1b[%zuA\r",
            rendered_suggestions
        );
    } else {
        printf("\r");
    }

    /*
     * Position the physical terminal cursor at the logical
     * cursor position.
     *
     * cursor is a UTF-8 byte offset into buffer.
     */
    printf("%s", prompt);
    fwrite(
        buffer,
        1,
        cursor,
        stdout
    );

    g_previous_suggestion_lines = rendered_suggestions;

    fflush(stdout);
}

int terminal_read_key(struct key_event *event)
{
    if(event == NULL) {
        return R_ERROR;
    }

    INPUT_RECORD record;
    DWORD records_read;

    while(true) {
        if(!ReadConsoleInputW(
            g_console_input,
            &record,
            1,
            &records_read
        )) {
            log_error(
                "Failed reading console input (error %lu).\n",
                GetLastError()
            );
            return R_ERROR;
        }

        if(records_read == 0) {
            continue;
        }

        if(record.EventType != KEY_EVENT) {
            continue;
        }

        KEY_EVENT_RECORD key = record.Event.KeyEvent;

        /*
         * Ignore key-up events.
         */
        if(!key.bKeyDown) {
            continue;
        }

        switch(key.wVirtualKeyCode) {
            case VK_RETURN:
                event->type = KEY_ENTER;
                break;

            case VK_BACK:
                event->type = KEY_BACKSPACE;
                break;

            case VK_TAB:
                event->type = KEY_TAB;
                break;

            case VK_ESCAPE:
                event->type = KEY_ESCAPE;
                break;

            case VK_LEFT:
                event->type = KEY_LEFT;
                break;

            case VK_RIGHT:
                event->type = KEY_RIGHT;
                break;

            case VK_HOME:
                event->type = KEY_HOME;
                break;

            case VK_END:
                event->type = KEY_END;
                break;

            case VK_UP:
                event->type = KEY_UP;
                break;

            case VK_DOWN:
                event->type = KEY_DOWN;
                break;

            default:
                goto character;
        }

        event->codepoint = 0;
        return R_OK;

        character:

        wchar_t wc = key.uChar.UnicodeChar;

        if(wc == L'\0') {
            /*
             * Modifier/function/arrow key we don't support yet.
             */
            continue;
        }

        event->type = KEY_CHARACTER;
        event->codepoint = (unsigned int)wc;

        return R_OK;
    }
}
#endif
