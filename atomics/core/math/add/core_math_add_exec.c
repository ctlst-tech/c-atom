#include "core_math_add.h"

void core_math_add_exec(
    const core_math_add_inputs_t *i,
    core_math_add_outputs_t *o
)
{
    o->output = (i->input0) + (i->input1);
}
