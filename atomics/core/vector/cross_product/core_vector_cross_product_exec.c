#include "core_vector_cross_product.h"

void core_vector_cross_product(const core_type_v3f64_t *v1,
                               const core_type_v3f64_t *v2,
                               core_type_v3f64_t *v) {
    v->x = v1->y * v2->z - v1->z * v2->y;
    v->y = v1->z * v2->x - v1->x * v2->z;
    v->z = v1->x * v2->y - v1->y * v2->x;
}

void core_vector_cross_product_exec(const core_vector_cross_product_inputs_t *i,
                                    core_vector_cross_product_outputs_t *o) {
    core_vector_cross_product(&i->v1, &i->v2, &o->v);
}
