#pragma once

#include "array.h"
#include "prelude.h"
#include "token.h"

int tokenize(DynHeapArray<Token>* token_buffer, const char* filepath);
