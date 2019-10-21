#pragma once

#include "array.h"
#include "prelude.h"
#include "token.h"

int qxc_tokenize(array<struct qxc_token>* token_buffer, const char* filepath);
