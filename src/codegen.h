#pragma once

#include <stdbool.h>
#include "ast.h"

// DECL_HASH_TABLE(qxc_local_stack_offsets, char*, int)

void generate_asm(struct qxc_program* program, const char* output_filepath);
