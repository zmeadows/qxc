#pragma once

#include <math.h>
#include <string.h>

#include "prelude.h"

// TODO: add optional stack storage?
// TODO: eventually, add string table and string references to avoid duplicating same
// string many times during compilation
class String {
    size_t m_length;
    char* m_contents;

public:
    char* cstr(void) { return m_contents; }

    String(void) = delete;

    String(const char* cstr) : m_length(strlen(cstr)), m_contents((char*)malloc(m_length))
    {
        strncpy(m_contents, cstr, m_length);
    }

    inline bool operator==(const String& str)
    {
        return strs_are_equal(m_contents, str.m_contents);
    }

    inline bool operator!=(const String& str)
    {
        return !strs_are_equal(m_contents, str.m_contents);
    }

    char* begin(void) { return m_contents; }
    char* end(void) { return m_contents + m_length; }
    const char* begin(void) const { return m_contents; }
    const char* end(void) const { return m_contents + m_length; }
};

// struct strbuf strbuf_empty()
// {
//     struct qxc_strbuf str;
//     str.contents = str.stack_data;
//     str.capacity = STRBUF_STACK_SIZE;
//     str.length = 0;
//     str.owning = false;
//     return str;
// }
//
// struct strbuf strbuf_new(const char* cstr)
// {
//     struct qxc_strbuf str;
//     str.contents = str.stack_data;
//     str.capacity = STRBUF_STACK_SIZE;
//     str.length = 0;
//     return str;
// }
//
// void qxc_strbuf_clear(struct qxc_strbuf* str) { str->length = 0; }
//
// void qxc_strbuf_free(struct qxc_strbuf* str)
// {
//     if (str->owning) {
//         free(str->contents);
//     }
//     str->contents = str->stack_data;
//     str->capacity = STRBUF_STACK_SIZE;
//     str->length = 0;
//     str->owning = false;
// }
//
// void qxc_strbuf_reserve(struct qxc_strbuf* str, size_t bytes)
// {
//     if (bytes <= str->capacity) return;
//     debug_print("reserving %zu bytes for strbuf\n", bytes);
//     str->contents = realloc(str->contents, bytes);
//     str->capacity = bytes;
// }
//
// void qxc_strbuf_copy(struct qxc_strbuf* source, struct qxc_strbuf* target)
// {
//     qxc_strbuf_reserve(target, source->length);
//     memcpy(target->contents, source->contents, source->length);
// }
//
// static void qxc_strbuf_maybe_extend(struct qxc_strbuf* str)
// {
//     // TODO: check for possible failures and size overflow
//     if (str->capacity == str->length) {
//         size_t new_capacity = (size_t)ceil(1.61803398875 * (double)str->capacity);
//         if (new_capacity < 4) new_capacity = 4;
//         debug_print("extending strbuf from old capacity (%zu) to (%zu)\n",
//         str->capacity,
//                     new_capacity);
//         str->contents = realloc(str->contents, new_capacity);
//         printf("finished realloc\n");
//         str->capacity = new_capacity;
//     }
// }
//
// void qxc_strbuf_append_char(struct qxc_strbuf* str, char c)
// {
//     qxc_strbuf_maybe_extend(str);
//     str->contents[str->length] = c;
//     str->length++;
// }
//
// bool qxc_strbuf_compare_cstr(const struct qxc_strbuf* qstr, const char* cstr)
// {
//     if (qstr->length != strlen(cstr)) return false;
//
//     for (size_t i = 0; i < qstr->length; i++) {
//         if (qstr->contents[i] != cstr[i]) return false;
//     }
//
//     return true;
// }
