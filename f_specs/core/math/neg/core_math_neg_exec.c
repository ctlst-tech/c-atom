#include "core_math_neg.h"

void core_math_neg_exec(
    const core_math_neg_inputs_t *i,
    core_math_neg_outputs_t *o
)
{
    o->output = -(i->input);
}
