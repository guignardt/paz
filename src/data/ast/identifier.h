#pragma once

#include "util/alloc.h"
#include "util/string.h"

typedef struct AstIdentifier {
    Range range;
    String string;
} AstIdentifier;
