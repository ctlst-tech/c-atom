#include "core_math_asin.h"

#include <math.h>

void core_math_asin_exec(
    const core_math_asin_inputs_t *i,
    core_math_asin_outputs_t *o
)
{
    if (i->input > 1.0) {
        o->output = M_PI_2;
    } else if (i->input < -1.0) {
        o->output = -M_PI_2;
    } else {
        o->output = asin(i->input);
    }
}
