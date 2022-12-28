#include "core_cont_quadrotor_mix.h"

static inline core_type_f64_t mix(
        core_type_f64_t t, core_type_f64_t l, core_type_f64_t c, core_type_f64_t r,
        core_type_f64_t kt, core_type_f64_t kl, core_type_f64_t kc, core_type_f64_t kr
        ) {

    core_type_f64_t out = t * kt + l * kl + c * kc + r * kr;
    out = out < 1.0 ? out : 1.0;
    out = out > 0.0 ? out : 0.0;

    return out;
}

void core_cont_quadrotor_mix_exec(
    const core_cont_quadrotor_mix_inputs_t *i,
    core_cont_quadrotor_mix_outputs_t *o,
    const core_cont_quadrotor_mix_params_t *p
)
{
    o->m1 = mix(i->transv, i->longit, i->collective, i->rudder,
                p->Kt_m1, p->Kl_m1, p->Kc, p->Kr_m1);
    o->m2 = mix(i->transv, i->longit, i->collective, i->rudder,
                p->Kt_m2, p->Kl_m2, p->Kc, p->Kr_m2);
    o->m3 = mix(i->transv, i->longit, i->collective, i->rudder,
                p->Kt_m3, p->Kl_m3, p->Kc, p->Kr_m3);
    o->m4 = mix(i->transv, i->longit, i->collective, i->rudder,
                p->Kt_m4, p->Kl_m4, p->Kc, p->Kr_m4);
}
