#pragma once

#include "array.h"
#include "prelude.h"
#include "token.h"

int qxc_tokenize(array<Token>* token_buffer, const char* filepath);
