#include "prelude.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool strs_are_equal(const char* strA, const char* strB) { return !strcmp(strA, strB); }

void strip_ext(char* fname)
{
    char* end = fname + strlen(fname);

    while (end > fname && *end != '.') {
        --end;
    }

    if (end > fname) {
        *end = '\0';
    }
}

// void qxc_error_reset(struct qxc_error* error)
// {
//     error->code = qxc_no_error;
//     error->line = -1;
//     error->column = -1;
//     error->filepath = NULL;
//     error->description = NULL;
// }
//
// void qxc_error_print(struct qxc_error* error)
// {
//     fprintf(stderr, "%s:%d:%d --> %s", error->filepath, error->line, error->column,
//             error->description);
// }
