//
// Created by goofy on 2/3/22.
//

#ifndef C_ATOM_FSMINST_H
#define C_ATOM_FSMINST_H

#include "fsm.h"
#include "function.h"

typedef struct fsminst {
    function_spec_t spec;
    fsm_t **fsms;
} fsminst_t;


fsm_rv_t fsminst_load(const char *path, fsminst_t **fsminst_rv);
void fsminst_get_handler(fsminst_t *fsm, function_handler_t *fh);

#endif //C_ATOM_FSMINST_H
