#include "core_math_acos.h"

#include <math.h>

void core_math_acos_exec(
    const core_math_acos_inputs_t *i,
    core_math_acos_outputs_t *o
)
{
    if (i->input > 1.0) {
        o->output = 0.0;
    } else if (i->input < -1.0) {
        o->output = M_PI;
    } else {
        o->output = acos(i->input);
    }
}
