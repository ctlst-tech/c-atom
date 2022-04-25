#include "core_math_max.h"

void core_math_max_exec(
    const core_math_max_inputs_t *i,
    core_math_max_outputs_t *o
)
{
    o->output = ((i->input0) >= (i->input1)) ? (i->input0) : (i->input1);
}
