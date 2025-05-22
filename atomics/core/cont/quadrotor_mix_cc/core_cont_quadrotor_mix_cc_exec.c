#include <math.h>

#include "core_cont_quadrotor_mix_cc.h"

void core_cont_quadrotor_mix_cc_exec(
    const core_cont_quadrotor_mix_cc_inputs_t *i,
    core_cont_quadrotor_mix_cc_outputs_t *o,
    const core_cont_quadrotor_mix_cc_params_t *p) {
    // Compute attitude control components for each motor
    const core_type_f64_t a1 = i->transv * p->Kt_m1 + i->longit * p->Kl_m1 + i->rudder * p->Kr_m1;
    const core_type_f64_t a2 = i->transv * p->Kt_m2 + i->longit * p->Kl_m2 + i->rudder * p->Kr_m2;
    const core_type_f64_t a3 = i->transv * p->Kt_m3 + i->longit * p->Kl_m3 + i->rudder * p->Kr_m3;
    const core_type_f64_t a4 = i->transv * p->Kt_m4 + i->longit * p->Kl_m4 + i->rudder * p->Kr_m4;

    // Find the maximum attitude control component
    const core_type_f64_t max_a = fmax(fmax(a1, a2), fmax(a3, a4));

    // Compute the maximum allowable collective to keep all motors <= 1.0
    const core_type_f64_t collective_max = (p->Kc > 0.0) ? (1.0 - max_a) / p->Kc : 0.0;

    // Clamp the collective input to prevent saturation
    const core_type_f64_t clamped_collective = fmin(i->collective, collective_max);

    // Compute final motor outputs, clamping to 0.0 if below
    o->m1 = fmax(a1 + clamped_collective * p->Kc, 0.0);
    o->m2 = fmax(a2 + clamped_collective * p->Kc, 0.0);
    o->m3 = fmax(a3 + clamped_collective * p->Kc, 0.0);
    o->m4 = fmax(a4 + clamped_collective * p->Kc, 0.0);
}
