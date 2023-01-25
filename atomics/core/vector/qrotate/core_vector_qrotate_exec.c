#include "core_vector_qrotate.h"

void core_vector_cross_product(const core_type_v3f64_t *v1,
                               const core_type_v3f64_t *v2,
                               core_type_v3f64_t *v);

 void core_vector_qrotate_exec(const core_vector_qrotate_inputs_t *i, core_vector_qrotate_outputs_t *o)
{
    core_type_v3f64_t t1;
    core_type_v3f64_t t2;
    core_type_v3f64_t q_im;

    q_im.x = i->q.x;
    q_im.y = i->q.y;
    q_im.z = i->q.z;

    core_vector_cross_product(&q_im, &i->v, &t1);
    t1.x *= 2;
    t1.y *= 2;
    t1.z *= 2;
    core_vector_cross_product(&q_im, &t1, &t2);

    o->v.x = t2.x + i->q.w * t1.x + i->v.x;
    o->v.y = t2.y + i->q.w * t1.y + i->v.y;
    o->v.z = t2.z + i->q.w * t1.z + i->v.z;
}

