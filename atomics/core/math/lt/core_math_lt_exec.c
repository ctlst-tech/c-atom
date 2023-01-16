#include "core_math_lt.h"

void core_math_lt_exec(
    const core_math_lt_inputs_t *i,
    core_math_lt_outputs_t *o
)
{
    o->output = (i->input0) < (i->input1);
}
