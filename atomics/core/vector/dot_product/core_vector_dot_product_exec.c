#include "core_vector_dot_product.h"

void core_vector_dot_product(const core_type_v3f64_t *v1,
                             const core_type_v3f64_t *v2, core_type_f64_t *o) {
    *o = v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

void core_vector_dot_product_exec(const core_vector_dot_product_inputs_t *i,
                                  core_vector_dot_product_outputs_t *o) {
    core_vector_dot_product(&i->v1, &i->v2, &o->output);
}
