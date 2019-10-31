#pragma once

#include "array.h"
#include "prelude.h"
#include "token.h"

int tokenize(DynArray<Token>* token_buffer, const char* filepath);
