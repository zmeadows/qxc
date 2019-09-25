#include "allocator.h"
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "prelude.h"
#include "pretty_print_ast.h"

#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LEN ((size_t)1024)

enum qxc_mode { qxc_tokenize_mode, qxc_parse_mode, qxc_compile_mode, qxc_unknown_mode };

int main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Need at least two arguments!\n");
        return EXIT_FAILURE;
    }

    const char* input_filepath = NULL;
    enum qxc_mode MODE = qxc_compile_mode;
    bool verbose = false;

    for (int i = 1; i < argc; i++) {
        const char* ith_arg = argv[i];

        if (access(ith_arg, R_OK) == -1) {
            if (strs_are_equal("-t", ith_arg)) {
                MODE = qxc_tokenize_mode;
            }
            else if (strs_are_equal("-p", ith_arg)) {
                MODE = qxc_parse_mode;
            }
            else if (strs_are_equal("-v", ith_arg)) {
                verbose = true;
            }
        }
        else {
            input_filepath = ith_arg;
        }
    }

    printf("input filepath: %s\n", input_filepath);

    if (input_filepath == NULL) {
        fprintf(stderr, "Failed to open file\n");
        return EXIT_FAILURE;
    }

    const size_t path_len = strlen(input_filepath);

    if (path_len > MAX_PATH_LEN - 1) {
        fprintf(stderr, "input filepath too long!\n");
    }

    char input_dir[MAX_PATH_LEN];
    char input_base[MAX_PATH_LEN];

    strncpy(input_dir, input_filepath, path_len);
    strncpy(input_base, input_filepath, path_len);

    char* dname = dirname(input_dir);
    char* bname = basename(input_base);
    strip_ext(bname);

    char output_assembly_path[MAX_PATH_LEN];
    char output_object_path[MAX_PATH_LEN];
    char output_exe_path[MAX_PATH_LEN];
    sprintf(output_assembly_path, "%s/%s.asm", dname, bname);
    sprintf(output_object_path, "%s/%s.o", dname, bname);
    sprintf(output_exe_path, "%s/%s", dname, bname);

    qxc_memory_reserve();

    if (MODE == qxc_tokenize_mode) {
        struct qxc_token_buffer* tokens = qxc_tokenize(input_filepath);
        if (tokens == NULL) {
            fprintf(stderr, "lexure failure\n");
            return EXIT_FAILURE;
        }
        printf("=== TOKENS ===\n");
        for (size_t i = 0; i < tokens->length; i++) {
            qxc_token_print(&tokens->tokens[i]);
        }
        return EXIT_SUCCESS;
    }

    struct qxc_program* program = qxc_parse(input_filepath);
    if (program == NULL) {
        return EXIT_FAILURE;
    }

    if (verbose) {
        print_program(program);
    }

    const bool codegen_success = generate_asm(program, output_assembly_path);

    if (!codegen_success) {
        return EXIT_FAILURE;
    }

    char nasm_cmd[MAX_PATH_LEN * 3];
    sprintf(nasm_cmd, "nasm -felf64 %s -o %s", output_assembly_path, output_object_path);
    if (system(nasm_cmd) != 0) {
        fprintf(stderr, "NASM ASSEMBLER FAILED\n");
        return EXIT_FAILURE;
    }

    char ld_cmd[MAX_PATH_LEN * 3];
    sprintf(ld_cmd, "ld %s -o %s", output_object_path, output_exe_path);

    if (system(ld_cmd) != 0) {
        fprintf(stderr, "LD LINKER FAILED\n");
        return EXIT_FAILURE;
    }

    qxc_memory_release();
}
