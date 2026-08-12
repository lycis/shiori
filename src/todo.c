#include "todo.h"

const char *todo_status_icon(todo_status status) {
    switch(status) {
        case OPEN:
            return "📌";

        case IN_PROGRESS:
            return "🚧";

        case DONE:
            return "✅";

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

        default:
            return "????";
    }
}