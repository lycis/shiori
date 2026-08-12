#include <stdlib.h>
#include "todo.h"
#include "logging.h"
#include "todo_list.h"

void todo_list_init(struct todo_list *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void todo_list_free(struct todo_list *list) {
    free(list->items);

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int todo_list_add(struct todo_list *list, const struct todo *item) {
    if(list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 8 : list->capacity * 2;

        struct todo *new_items = realloc(
            list->items,
            new_capacity * sizeof(struct todo)
        );

        if(new_items == NULL) {
            log_error("Failed allocating TODO list.\n");
            return R_ERROR;
        }

        list->items = new_items;
        list->capacity = new_capacity;
    }

    list->items[list->count] = *item;
    list->count++;

    return R_OK;
}