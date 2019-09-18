#include "allocator.h"
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "pretty_print_ast.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    assert(argc == 2);

    if (access(argv[1], R_OK) == -1) {
        fprintf(stderr, "Failed to open file\n");
        return EXIT_FAILURE;
    }

    qxc_memory_reserve();

    // struct qxc_token_buffer* tokens = qxc_tokenize(argv[1]);
    // printf("=== TOKENS ===\n");
    // for (size_t i = 0; i < tokens->length; i++) {
    //     qxc_token_print(&tokens->tokens[i]);
    // }

    struct qxc_program* program = qxc_parse(argv[1]);
    printf("ast ptr = %p\n", (void*)program);
    print_program(program);

    generate_asm(program, "assembly.s");

    qxc_memory_release();
}
