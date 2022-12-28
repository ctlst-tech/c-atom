#include "core_math_in_range.h"

void core_math_in_range_exec(
    const core_math_in_range_inputs_t *i,
    core_math_in_range_outputs_t *o
)
{
    o->output = ((i->min) <= (i->input)) && ((i->input) <= (i->max));
}
