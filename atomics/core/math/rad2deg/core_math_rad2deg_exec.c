#include "core_math_rad2deg.h"
#include <math.h>

void core_math_rad2deg_exec(const core_math_rad2deg_inputs_t *i, core_math_rad2deg_outputs_t *o)
{
    o->output = i->input * 180.0 / M_PI;
}
