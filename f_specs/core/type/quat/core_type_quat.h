#ifndef FSPEC_CORE_TYPE_QUAT_H
#define FSPEC_CORE_TYPE_QUAT_H

/* Include declaration of dependency types */
#include "core_type_f64.h"

/**
 * @brief Quaternion
 * Quaternion that can represent a rotation about an axis in 3-D space.
 */
typedef struct core_type_quat
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

} core_type_quat_t;

#endif // FSPEC_CORE_TYPE_QUAT_H
