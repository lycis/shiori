#ifndef _SHIORI_TODOLIST_H
#define _SHIORI_TODOLIST_H
struct todo_list {
    struct todo *items;
    size_t count;
    size_t capacity;
};
void todo_list_init(struct todo_list *list);
void todo_list_free(struct todo_list *list);
int todo_list_add(struct todo_list *list, const struct todo *item);
struct todo *todo_list_find_by_id(struct todo_list *list, unsigned long long id);
#endif