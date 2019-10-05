#include "allocator.h"
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "prelude.h"
#include "pretty_print_ast.h"

#include <assert.h>
#include <libgen.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum qxc_mode { TOKENIZE_MODE, PARSE_MODE, COMPILE_MODE };

struct qxc_context {
    char canonical_input_filepath[PATH_MAX];
    char work_dir[PATH_MAX];
    char output_assembly_path[2 * PATH_MAX];
    char output_object_path[2 * PATH_MAX];
    char output_exe_path[2 * PATH_MAX];

    enum qxc_mode mode;
    bool verbose;
};

// parse command line arguments, determine paths of output files and working directory
static int qxc_context_init(struct qxc_context* ctx, int argc, char* argv[])
{
    ctx->canonical_input_filepath[0] = '\0';
    ctx->work_dir[0] = '\0';
    ctx->output_assembly_path[0] = '\0';
    ctx->output_object_path[0] = '\0';
    ctx->output_exe_path[0] = '\0';

    ctx->mode = COMPILE_MODE;
    ctx->verbose = false;

    if (argc < 2) {
        // TODO error print macro
        fprintf(stderr,
                "Need at least one argument specifying .c file to be compiled.\n");
        return -1;
    }

    const char* user_specified_input_filepath = NULL;

    for (int i = 1; i < argc; i++) {
        const char* ith_arg = argv[i];

        if (access(ith_arg, R_OK) == -1) {
            if (strs_are_equal("-t", ith_arg)) {
                ctx->mode = TOKENIZE_MODE;
            }
            else if (strs_are_equal("-p", ith_arg)) {
                ctx->mode = PARSE_MODE;
            }
            else if (strs_are_equal("-v", ith_arg)) {
                ctx->verbose = true;
            }
        }
        else {
            user_specified_input_filepath = ith_arg;
        }
    }

    if (user_specified_input_filepath == NULL) {
        fprintf(stderr, "No input file specified in command line arguments.\n");
        return EXIT_FAILURE;
    }

    if (realpath(user_specified_input_filepath, ctx->canonical_input_filepath) == NULL) {
        fprintf(stderr, "Specified file doesn't exist or could not be opened.\n");
        fprintf(stderr, "--> %s\n", user_specified_input_filepath);
        return EXIT_FAILURE;
    }

    const size_t canonical_path_len = strlen(ctx->canonical_input_filepath);

    char input_dir[PATH_MAX];
    char input_base[PATH_MAX];
    strncpy(input_dir, ctx->canonical_input_filepath, canonical_path_len);
    strncpy(input_base, ctx->canonical_input_filepath, canonical_path_len);

    char* dname = dirname(input_dir);
    char* bname = basename(input_base);
    strip_ext(bname);

    const char* new_tmp_dir = mk_tmp_dir();
    if (new_tmp_dir == NULL) {
        fprintf(stderr, "failed to create working dir for qxc\n");
        return -1;
    }

    sprintf(ctx->work_dir, "%s", new_tmp_dir);

    sprintf(ctx->output_assembly_path, "%s/%s.asm", ctx->work_dir, bname);
    sprintf(ctx->output_object_path, "%s/%s.o", ctx->work_dir, bname);
    sprintf(ctx->output_exe_path, "%s/%s", dname, bname);

    return 0;
}

static void qxc_context_deinit(struct qxc_context* ctx) { rm_tmp_dir(ctx->work_dir); }

static int qxc_context_run(const struct qxc_context* ctx)
{
    if (ctx->mode == TOKENIZE_MODE) {
        struct qxc_memory_pool* tmp_pool = qxc_memory_pool_init(10e3);
        struct qxc_token_array* tokens =
            qxc_tokenize(tmp_pool, ctx->canonical_input_filepath);
        if (tokens == NULL) {
            fprintf(stderr, "lexure failure\n");
            qxc_memory_pool_release(tmp_pool);
            return -1;
        }
        printf("=== TOKENS ===\n");
        for (size_t i = 0; i < tokens->length; i++) {
            qxc_token_print(qxc_token_array_at(tokens, i));
        }
        qxc_memory_pool_release(tmp_pool);
        return 0;
    }

    struct qxc_program* program = qxc_parse(ctx->canonical_input_filepath);
    if (program == NULL) {
        return -1;
    }

    // printf("%p\n", (void*)program->main_decl->slist);
    // if (ctx->verbose) {
    //     print_program(program);
    // }
    // printf("%p\n", (void*)program->main_decl->slist);

    generate_asm(program, ctx->output_assembly_path);

    char nasm_cmd[PATH_MAX * 5];
    sprintf(nasm_cmd, "nasm -felf64 %s -o %s", ctx->output_assembly_path,
            ctx->output_object_path);
    if (system(nasm_cmd) != 0) {
        fprintf(stderr, "NASM ASSEMBLER FAILED\n");
        return -1;
    }

    char ld_cmd[PATH_MAX * 5];
    sprintf(ld_cmd, "ld %s -o %s", ctx->output_object_path, ctx->output_exe_path);

    if (system(ld_cmd) != 0) {
        fprintf(stderr, "LD LINKER FAILED\n");
        return -1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    struct qxc_context ctx;

    const int init_code = qxc_context_init(&ctx, argc, argv);
    if (init_code != 0) {
        qxc_context_deinit(&ctx);
        return init_code;
    }

    const int run_code = qxc_context_run(&ctx);
    if (run_code != 0) {
        qxc_context_deinit(&ctx);
        return run_code;
    }

    qxc_context_deinit(&ctx);
    return EXIT_SUCCESS;
}
