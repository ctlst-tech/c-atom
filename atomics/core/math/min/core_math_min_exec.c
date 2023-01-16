#include "core_math_min.h"

void core_math_min_exec(
    const core_math_min_inputs_t *i,
    core_math_min_outputs_t *o
)
{
    o->output = ((i->input0) <= (i->input1)) ? (i->input0) : (i->input1);
}
