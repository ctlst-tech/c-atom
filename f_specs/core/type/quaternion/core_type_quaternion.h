#ifndef CTLST_CORE_TYPE_QUATERNION_H
#define CTLST_CORE_TYPE_QUATERNION_H

/* Include declaration of dependency types */
#include "core_type_f64.h"

/**
 * @brief Quaternion
 * Quaternion that can represent a rotation about an axis in 3-D space.
 */
typedef struct core_type_quaternion_
{
    /**
     * @brief The quaternion's W-component
     */
    core_type_f64_t w;

    /**
     * @brief The quaternion's X-component
     */
    core_type_f64_t x;

    /**
     * @brief The quaternion's Y-component
     */
    core_type_f64_t y;

    /**
     * @brief The quaternion's Z-component
     */
    core_type_f64_t z;

} core_type_quaternion_t;

#endif // CTLST_CORE_TYPE_QUATERNION_H
