#include "core_math_sub.h"

void core_math_sub_exec(
    const core_math_sub_inputs_t *i,
    core_math_sub_outputs_t *o
)
{
    o->output = (i->input0) - (i->input1);
}
