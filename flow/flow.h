//
// Created by goofy on 2/5/22.
//

#ifndef C_ATOM_FLOW_H
#define C_ATOM_FLOW_H

#include "function.h"

void flow_get_handler(function_flow_t *flow, function_handler_t *fh);

fspec_rv_t flow_load(const char *path, function_flow_t **flow_rv);
fspec_rv_t flow_reg_find_handler(const char *fname, function_handler_t **flow_rv);

#endif //C_ATOM_FLOW_H
