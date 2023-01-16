#include "core_logical_nor.h"

void core_logical_nor_exec(
    const core_logical_nor_inputs_t *i,
    core_logical_nor_outputs_t *o
)
{
    o->output = !((i->input0) || (i->input1));
}
