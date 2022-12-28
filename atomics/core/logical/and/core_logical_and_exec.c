#include "core_logical_and.h"

void core_logical_and_exec(
    const core_logical_and_inputs_t *i,
    core_logical_and_outputs_t *o
)
{
    o->output = (i->input0) && (i->input1);
}
