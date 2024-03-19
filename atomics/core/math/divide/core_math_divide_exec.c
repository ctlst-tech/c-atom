#include "core_math_divide.h"

void core_math_divide_exec(const core_math_divide_inputs_t *i,
                           core_math_divide_outputs_t *o,
                           const core_math_divide_params_t *p) {
    if (fabs(i->divisior) < p->min_divisior) {
        o->output = 0;
    } else {
        o->output = i->dividend / i->divisior;
    }
}
