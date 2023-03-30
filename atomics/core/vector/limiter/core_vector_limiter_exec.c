#include <math.h>
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
//    o->v.x = value_limiter(i->v.x, p->min, p->max);
//    o->v.y = value_limiter(i->v.y, p->min, p->max);
//    o->v.z = value_limiter(i->v.z, p->min, p->max);
    core_type_f64_t magn = sqrt(i->v.x * i->v.x + i->v.y * i->v.y);

    if (magn > p->max) {
        core_type_f64_t f = p->max / magn;
        o->v.x = f * i->v.x;
        o->v.y = f * i->v.y;
    } else {
        o->v.x = i->v.x;
        o->v.y = i->v.y;
    }

    o->v.z = value_limiter(i->v.z, p->min, p->max);
}
