#pragma once

#include "util/alloc.h"
#include "data/token.h"

void tokenize(char const* text, OUT(TokenStream) dst);
