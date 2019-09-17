#include "allocator.h"
#include "ast.h"
#include "lexer.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    assert(argc == 2);
    qxc_memory_reserve();

    // struct qxc_token_buffer* tokens = qxc_tokenize(argv[1]);
    // printf("=== TOKENS ===\n");
    // for (size_t i = 0; i < tokens->length; i++) {
    //     qxc_token_print(&tokens->tokens[i]);
    // }

    struct qxc_program* ast = qxc_parse(argv[1]);
    printf("ast ptr = %p\n", (void*)ast);

    qxc_memory_release();
}
