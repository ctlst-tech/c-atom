#include <math.h>

#include "core_quat_euler_correction.h"


void quat_product ( const core_type_quat_t * ql, const core_type_quat_t* qr, core_type_quat_t* out ) {
    core_type_quat_t tmp;
    tmp.w = qr->w * ql->w - qr->x * ql->x - qr->y * ql->y - qr->z * ql->z;
    tmp.x = qr->w * ql->x + qr->x * ql->w - qr->y * ql->z + qr->z * ql->y;
    tmp.y = qr->w * ql->y + qr->x * ql->z + qr->y * ql->w - qr->z * ql->x;
    tmp.z = qr->w * ql->z - qr->x * ql->y + qr->y * ql->x + qr->z * ql->w;
    *out = tmp;
}

void core_quat_euler_correction_exec(
    const core_quat_euler_correction_inputs_t *i,
    core_quat_euler_correction_outputs_t *o
)
{
    core_type_quat_t q_err;
    core_type_quat_t q_out;

    q_out = i->q;

    if (fabs(i->yaw_err) > 0.0) {
        q_err.w = cos(-i->yaw_err * 0.5);
        q_err.x = 0;
        q_err.y = 0;
        q_err.z = sin(-i->yaw_err * 0.5);

        quat_product(&q_err, &i->q, &q_out);
    }

    if ((fabs(i->roll_err) > 0.0) || (fabs(i->pitch_err) > 0.0)) {
        q_err.w = cos(-i->roll_err * 0.5);
        q_err.x = sin(-i->roll_err * 0.5);;
        q_err.y = 0;
        q_err.z = 0;

        core_type_quat_t q_err2;
        core_type_f64_t sin_err_pitch = sin(-i->pitch_err * 0.5);
        q_err2.w = cos(-i->pitch_err * 0.5);;
        q_err2.x = 0;
        q_err2.y =  sin_err_pitch * cos(-i->roll);
        q_err2.z = -sin_err_pitch * sin(-i->roll);

        quat_product(&q_err2, &q_err, &q_err);
        quat_product(&q_out, &q_err, &q_out);
    }

    o->q = q_out;
}
