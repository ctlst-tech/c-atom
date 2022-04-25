#include "core_math_clamp.h"

void core_math_clamp_exec(
    const core_math_clamp_inputs_t *i,
    core_math_clamp_outputs_t *o
)
{
    if (i->input <= i->min) {
        o->output = i->min;
    } else if (i->input >= i->max) {
        o->output = i->max;
    } else {
        o->output = i->input;
    }
}
