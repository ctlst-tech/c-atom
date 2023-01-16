#include <math.h>
#include "core_quat_to_euler.h"

#define cmp(a,b,sign) ((a) sign (b)) ? (a) : (b)

#define min(a,b) ((a) < (b)) ? (a) : (b)
#define max(a,b) ((a) > (b)) ? (a) : (b)

#define rad2deg(r)  ((r) * 180.0 / M_PI)

void core_quat_to_euler_exec(const core_quat_to_euler_inputs_t *i, core_quat_to_euler_outputs_t *o)
{
#define square(s) ((s)*(s))
    core_type_f64_t asin_arg = 2*( i->q.w * i->q.y - i->q.z * i->q.x);
    core_type_f64_t q2_sq = square (i->q.y);

    asin_arg = max ( -1.0, asin_arg );
    asin_arg = min (  1.0, asin_arg );
    
    o->roll = atan2( 2 * (i->q.w * i->q.x + i->q.y*i->q.z), 1 - 2 * ( square ( i->q.x) + q2_sq ) );
    o->pitch = asin( asin_arg );
    o->yaw = atan2( 2 * (i->q.w * i->q.z + i->q.x * i->q.y), 1 - 2 * ( q2_sq + square (i->q.z) ) );

    // printf("R=%.2f P=%.2f Y=%.2f\n", rad2deg(o->roll), rad2deg(o->pitch), rad2deg(o->yaw));
}
