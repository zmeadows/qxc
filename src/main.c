#include "allocator.h"
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "pretty_print_ast.h"

#include <assert.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LEN ((size_t)1024)

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

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments, should be 2.\n");
        return EXIT_FAILURE;
    }

    const char* input_filepath = argv[1];

    if (access(input_filepath, R_OK) == -1) {
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

    // struct qxc_token_buffer* tokens = qxc_tokenize(argv[1]);
    // printf("=== TOKENS ===\n");
    // for (size_t i = 0; i < tokens->length; i++) {
    //     qxc_token_print(&tokens->tokens[i]);
    // }

    struct qxc_program* program = qxc_parse(input_filepath);
    if (program == NULL) {
        return EXIT_FAILURE;
    }
    // print_program(program);

    bool codegen_success = generate_asm(program, output_assembly_path);

    if (codegen_success) {
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
    }

    qxc_memory_release();
}
