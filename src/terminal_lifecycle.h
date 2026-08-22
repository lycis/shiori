#ifndef SHIORI_TERMINAL_LIFECYCLE_H
#define SHIORI_TERMINAL_LIFECYCLE_H

#include <stdatomic.h>
#include <stdbool.h>

enum terminal_lifecycle_phase {
    TERMINAL_LIFECYCLE_INACTIVE,
    TERMINAL_LIFECYCLE_ACTIVE,
    TERMINAL_LIFECYCLE_CLEANING
};

enum terminal_lifecycle_enter_result {
    TERMINAL_LIFECYCLE_ENTER_FAILED = -1,
    TERMINAL_LIFECYCLE_ENTER_FIRST,
    TERMINAL_LIFECYCLE_ENTER_NESTED
};

struct terminal_lifecycle {
    atomic_int phase;
    atomic_size_t depth;
};

enum terminal_lifecycle_enter_result terminal_lifecycle_enter(struct terminal_lifecycle *lifecycle);
bool terminal_lifecycle_leave(struct terminal_lifecycle *lifecycle);
bool terminal_lifecycle_claim_cleanup(struct terminal_lifecycle *lifecycle);
void terminal_lifecycle_complete_cleanup(struct terminal_lifecycle *lifecycle);

#endif
