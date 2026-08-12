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

struct todo *todo_list_find_by_id(struct todo_list *list, unsigned long long id) {
    for(size_t i = 0; i < list->count; ++i) {
        if(list->items[i].id == id) {
            return &list->items[i];
        }
    }

    return NULL;
}

int todo_list_remove_by_id(struct todo_list *list, unsigned long long id) {
    for(size_t i = 0; i < list->count; ++i) {
        if(list->items[i].id != id) {
            continue;
        }

        for(size_t j = i; j + 1 < list->count; ++j) {
            list->items[j] = list->items[j + 1];
        }

        list->count--;

        return R_OK;
    }

    return R_ERROR;
}