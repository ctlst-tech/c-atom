#include "core_math_deg2rad.h"
#include <math.h>

void core_math_deg2rad_exec(const core_math_deg2rad_inputs_t *i, core_math_deg2rad_outputs_t *o)
{
    o->output = i->input * M_PI / 180.0;
}
