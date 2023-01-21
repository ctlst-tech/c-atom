#ifndef FSPEC_CORE_TYPE_VECTOR_U8_H
#define FSPEC_CORE_TYPE_VECTOR_U8_H

#include <stdint.h>

#include "core_type_u8.h"

/**
 * @brief Vector of 8-bit unsigned integral number
 */
typedef struct core_type_vector_u8{
    core_type_u8_t* vector;
    uint32_t curr_len;
    uint32_t max_len;
} core_type_vector_u8_t;

#endif // FSPEC_CORE_TYPE_VECTOR_U8_H
