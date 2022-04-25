//
// Created by Ivan Makarov on 30/1/22.
//

#include <stdio.h>
#include <stdarg.h>

#include "error.h"

int error(const char *message, ... ) {
    va_list args;
    va_start (args,message);

    vfprintf(stderr, message, args);
    fprintf(stderr, "\n");

    va_end (args);

    return 0;
}
