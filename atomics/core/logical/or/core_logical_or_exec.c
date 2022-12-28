#include "core_logical_or.h"

void core_logical_or_exec(
    const core_logical_or_inputs_t *i,
    core_logical_or_outputs_t *o
)
{
    o->output = (i->input0) || (i->input1);
}
