#ifndef FSPEC_CORE_TYPE_VECTOR_F64_H
#define FSPEC_CORE_TYPE_VECTOR_F64_H

#include <stdint.h>

#include "core_type_f64.h"

/**
 * @brief Vector of double precision numbers
 */
typedef struct core_type_vector_f64{
    core_type_f64_t* vector;
    uint32_t curr_len;
    uint32_t max_len;
} core_type_vector_f64_t;

#endif // FSPEC_CORE_TYPE_VECTOR_F64_H
