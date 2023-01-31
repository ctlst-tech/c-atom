#include "core_math_sub_rad_angle.h"
#include <math.h>

void core_math_sub_rad_angle_exec(const core_math_sub_rad_angle_inputs_t *i, core_math_sub_rad_angle_outputs_t *o)
{
    core_type_f64_t sub = (i->input0) - (i->input1);

    while ( sub > M_PI ) {
        sub -= 2*M_PI;
    }
    while ( sub < -M_PI ) {
        sub += 2*M_PI;
    }

    o->output = sub;
}
