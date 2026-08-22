#include "terminal_lifecycle.h"

#include <stdbool.h>
#include <stddef.h>

enum terminal_lifecycle_enter_result terminal_lifecycle_enter(struct terminal_lifecycle *lifecycle) {
    if(lifecycle == NULL) {
        return TERMINAL_LIFECYCLE_ENTER_FAILED;
    }

    int expected = TERMINAL_LIFECYCLE_INACTIVE;

    if(atomic_compare_exchange_strong(&lifecycle->phase, &expected, TERMINAL_LIFECYCLE_ACTIVE)) {
        atomic_store(&lifecycle->depth, 1);
        return TERMINAL_LIFECYCLE_ENTER_FIRST;
    }

    if(expected != TERMINAL_LIFECYCLE_ACTIVE) {
        return TERMINAL_LIFECYCLE_ENTER_FAILED;
    }

    atomic_fetch_add(&lifecycle->depth, 1);

    if(atomic_load(&lifecycle->phase) != TERMINAL_LIFECYCLE_ACTIVE) {
        atomic_fetch_sub(&lifecycle->depth, 1);
        return TERMINAL_LIFECYCLE_ENTER_FAILED;
    }

    return TERMINAL_LIFECYCLE_ENTER_NESTED;
}

bool terminal_lifecycle_claim_cleanup(struct terminal_lifecycle *lifecycle) {
    if(lifecycle == NULL) {
        return false;
    }

    int expected = TERMINAL_LIFECYCLE_ACTIVE;
    return atomic_compare_exchange_strong(&lifecycle->phase, &expected, TERMINAL_LIFECYCLE_CLEANING);
}

bool terminal_lifecycle_leave(struct terminal_lifecycle *lifecycle) {
    if(lifecycle == NULL || atomic_load(&lifecycle->phase) != TERMINAL_LIFECYCLE_ACTIVE) {
        return false;
    }

    size_t depth = atomic_load(&lifecycle->depth);

    while(depth > 0) {
        if(atomic_compare_exchange_weak(&lifecycle->depth, &depth, depth - 1)) {
            return depth == 1 && terminal_lifecycle_claim_cleanup(lifecycle);
        }
    }

    return false;
}

void terminal_lifecycle_complete_cleanup(struct terminal_lifecycle *lifecycle) {
    if(lifecycle == NULL) {
        return;
    }

    atomic_store(&lifecycle->depth, 0);
    atomic_store(&lifecycle->phase, TERMINAL_LIFECYCLE_INACTIVE);
}
