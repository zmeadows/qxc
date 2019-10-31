#pragma once

#include <stdbool.h>

#include "ast.h"

void generate_asm(Program* program, const char* output_filepath);
