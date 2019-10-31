#include "files.h"

#include <linux/limits.h>

#include "allocator.h"
#include "prelude.h"

// struct file_contents {
//     const char* filepath;
//     const char* const contents;
// };
//
// static struct file_contents* s_files = nullptr;
// static size_t s_files_count = 0;
// static size_t s_files_capacity = 0;
//
// const char* qxc_get_file_contents(const char* filepath)
// {
//     // OPTIMIZE: hash table with filepath keys would be better for large projects
//     for (size_t i = 0; i < s_files_count; i++) {
//         if (strs_are_equal(absolute_path, s_files[i].filepath)) {
//             return s_files[i].content;
//         }
//     }
//
//     struct file_contents* new_file = extend_file_buffer();
//
//     FILE* f = fopen(filepath, "r");
//
//     if (f == nullptr) {
//         return nullptr;
//     }
//
//     // get the file size in bytes
//     fseek(f, 0, SEEK_END);
//     long fsizel = ftell(f);
//
//     if (fsizel < 0) {
//         return nullptr;
//     }
//
//     size_t fsize = (size_t)fsizel;
//
//     rewind(f);
//
//     tokenizer->contents = malloc(fsize + 1);
//     fread(tokenizer->contents, 1, fsize, f);
//     fclose(f);
// }
//
