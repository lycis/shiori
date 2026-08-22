#include <stdio.h>

#include "terminal_lifecycle.h"

static int expect(bool condition, const char *message) {
    if(condition) {
        return 0;
    }

    fprintf(stderr, "%s\n", message);
    return 1;
}

int main(void) {
    struct terminal_lifecycle lifecycle = {0};

    if(expect(
           terminal_lifecycle_enter(&lifecycle) == TERMINAL_LIFECYCLE_ENTER_FIRST,
           "first entry was not recognized"
       ) ||
       expect(
           terminal_lifecycle_enter(&lifecycle) == TERMINAL_LIFECYCLE_ENTER_NESTED,
           "nested entry was not recognized"
       ) ||
       expect(!terminal_lifecycle_leave(&lifecycle), "nested leave claimed cleanup") ||
       expect(terminal_lifecycle_leave(&lifecycle), "final leave did not claim cleanup") ||
       expect(!terminal_lifecycle_leave(&lifecycle), "cleanup was claimed twice") ||
       expect(!terminal_lifecycle_claim_cleanup(&lifecycle), "cleaning lifecycle was claimed twice")) {
        return 1;
    }

    terminal_lifecycle_complete_cleanup(&lifecycle);

    if(expect(
           terminal_lifecycle_enter(&lifecycle) == TERMINAL_LIFECYCLE_ENTER_FIRST,
           "lifecycle could not be reused after cleanup"
       ) ||
       expect(terminal_lifecycle_claim_cleanup(&lifecycle), "interrupt could not claim cleanup") ||
       expect(!terminal_lifecycle_leave(&lifecycle), "normal leave raced through interrupt cleanup") ||
       expect(!terminal_lifecycle_claim_cleanup(&lifecycle), "interrupt cleanup was claimed twice")) {
        return 1;
    }

    terminal_lifecycle_complete_cleanup(&lifecycle);

    return expect(
        terminal_lifecycle_enter(NULL) == TERMINAL_LIFECYCLE_ENTER_FAILED,
        "null lifecycle entry did not fail"
    );
}
