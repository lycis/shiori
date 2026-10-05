#include "todo.h"

#include <string.h>

const char *todo_priority_string(enum todo_priority priority) {
    switch(priority) {
    case TODO_PRIORITY_HIGH:
        return "high";
    case TODO_PRIORITY_MEDIUM:
        return "medium";
    case TODO_PRIORITY_LOW:
        return "low";
    default:
        return "none";
    }
}

int parse_todo_priority(const char *value, enum todo_priority *priority) {
    if(value == nullptr || priority == nullptr) {
        return R_ERROR;
    }
    static const enum todo_priority priorities[] =
        {TODO_PRIORITY_NONE, TODO_PRIORITY_HIGH, TODO_PRIORITY_MEDIUM, TODO_PRIORITY_LOW};
    for(size_t i = 0; i < sizeof(priorities) / sizeof(priorities[0]); ++i) {
        if(strcmp(value, todo_priority_string(priorities[i])) == 0) {
            *priority = priorities[i];
            return R_OK;
        }
    }
    return R_ERROR;
}

const char *todo_status_icon(todo_status status) {
    switch(status) {
    case OPEN:
        return "📌";

    case IN_PROGRESS:
        return "🚧";

    case DONE:
        return "✅";

    case CANCELLED:
        return "🚫";

    case DEFERRED:
        return "⏸️";

    default:
        return "❓";
    }
}

int format_todo_date(time_t timestamp, char *buffer, size_t size) {
    struct tm local_time;

    if(localtime_s(&local_time, &timestamp) != 0) {
        return R_ERROR;
    }

    if(strftime(buffer, size, "%Y-%m-%d", &local_time) == 0) {
        return R_ERROR;
    }

    return R_OK;
}

const char *todo_status_mark(todo_status status) {
    switch(status) {
    case OPEN:
        return " ";

    case IN_PROGRESS:
        return "/";

    case DONE:
        return "x";

    case CANCELLED:
        return "-";

    case DEFERRED:
        return ">";

    default:
        return "?";
    }
}

const char *todo_status_string(todo_status status) {
    switch(status) {
    case OPEN:
        return "OPEN";

    case IN_PROGRESS:
        return "IN PROGRESS";

    case DONE:
        return "DONE";

    case CANCELLED:
        return "CANCELLED";

    case DEFERRED:
        return "DEFERRED";

    default:
        return "????";
    }
}

const char *todo_status_simple_icon(todo_status status) {
    switch(status) {
    case OPEN:
        return "·";

    case IN_PROGRESS:
        return "›";

    case DONE:
        return "✓";

    case CANCELLED:
        return "×";

    case DEFERRED:
        return "»";

    default:
        return "????";
    }
}
