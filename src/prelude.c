#include "prelude.h"

#include <stdlib.h>
#include <string.h>

bool strs_are_equal(const char* strA, const char* strB) {
    return !strcmp(strA, strB);
}

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
