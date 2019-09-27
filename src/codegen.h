#pragma once

#include <stdbool.h>
#include "ast.h"

void generate_asm(struct qxc_program* program, const char* output_filepath);
