#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "todo.h"
bool g_debug_enabled = false;
#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        if(!(condition)) {                                                                                             \
            fprintf(stderr, "Failed at line %d\n", __LINE__);                                                          \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while(false)
int main(void) {
    enum todo_priority priority = TODO_PRIORITY_LOW;
    const char *values[] = {"none", "high", "medium", "low"};
    for(size_t i = 0; i < 4; ++i) {
        CHECK(parse_todo_priority(values[i], &priority) == R_OK);
        CHECK(strcmp(todo_priority_string(priority), values[i]) == 0);
    }
    CHECK(parse_todo_priority("urgent", &priority) == R_ERROR);
    CHECK(priority == TODO_PRIORITY_LOW);
    CHECK(parse_todo_priority("", &priority) == R_ERROR);
    CHECK(parse_todo_priority("HIGH", &priority) == R_ERROR);
    CHECK(parse_todo_priority(nullptr, &priority) == R_ERROR);
    char *args[] = {"--priority", "m"};
    struct completion_result result = complete_todo_priority(2, args, false);
    CHECK(result.count == 1 && strcmp(result.items[0], "medium") == 0);
    result = complete_todo_priority(1, args, true);
    CHECK(result.count == 4);
    char *option[] = {"--pri"};
    result = complete_todo_priority(1, option, false);
    CHECK(result.count == 1 && strcmp(result.items[0], "--priority") == 0);
    char *text[] = {"text"};
    CHECK(complete_todo_priority(1, text, false).count == 0);
    char *topic[] = {"--topic", "--pri"};
    CHECK(complete_todo_priority(2, topic, false).count == 0);
    CHECK(complete_todo_priority(0, nullptr, true).count == 2);
    CHECK(complete_todo_priority(1, text, true).count == 2);
    char *done[] = {"--priority", "high", "--pri"};
    CHECK(complete_todo_priority(3, done, false).count == 0);
    char *tag[] = {"--tag", "--priority", "--pri"};
    CHECK(complete_todo_priority(3, tag, false).count == 1);
    return 0;
}
