#ifndef FSPEC_CORE_TYPE_VECTOR3F64_H
#define FSPEC_CORE_TYPE_VECTOR3F64_H

/* Include declaration of dependency types */
#include "core_type_f64.h"

/**
 * @brief 3d vector of f64 type
 */
typedef struct core_type_vector3f64
{
    /**
     * @brief The vectors's X-component
     */
    core_type_f64_t x;

    /**
     * @brief The vectors's y-component
     */
    core_type_f64_t y;

    /**
     * @brief The vectors's z-component
     */
    core_type_f64_t z;

} core_type_vector3f64_t;

#endif // FSPEC_CORE_TYPE_VECTOR3F64_H
