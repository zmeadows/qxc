#pragma once

#include <stdbool.h>
#include "ast.h"

bool generate_asm(struct qxc_program* program, const char* output_filepath);
