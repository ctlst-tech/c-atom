#include "core_vector_limiter.h"

double value_limiter(double in, double min, double max) {
    double out;
    if (in < min) {
        out = min;
    } else if (in > max) {
        out = max;
    } else {
        out = in;
    }
    return out;
}

void core_vector_limiter_exec(
    const core_vector_limiter_inputs_t *i,
    core_vector_limiter_outputs_t *o,
    const core_vector_limiter_params_t *p) {
    o->v.x = value_limiter(i->v.x, p->min, p->max);
    o->v.y = value_limiter(i->v.y, p->min, p->max);
    o->v.z = value_limiter(i->v.z, p->min, p->max);
}
