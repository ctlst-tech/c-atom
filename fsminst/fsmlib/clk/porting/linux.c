//
// Created by goofy on 9/23/21.
//

#include <stdlib.h>
#include <pthread.h>
#include "clk.h"

clk_rv_t clk_drv_linux_init (void **dh);
clk_rv_t clk_drv_linux_emit_edge (void *dh, clk_src_t *src, int set_status_bit);
clk_rv_t clk_drv_linux_clear_status (void *dh, clk_src_t *src);
clk_rv_t clk_drv_linux_mux_point (void *dh);

const clk_mux_driver_t clk_mux_driver_linux = {
    .init = clk_drv_linux_init,
    .mux_point = clk_drv_linux_mux_point,
    .emit_edge = clk_drv_linux_emit_edge,
    .clear_status = clk_drv_linux_clear_status
};

typedef struct clk_driver_dh_linux {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    //int             counter;
} clk_driver_dh_linux_t;

clk_rv_t clk_drv_linux_init (void **dh) {
    clk_driver_dh_linux_t *d = calloc(1, sizeof(clk_driver_dh_linux_t));
    if(d == NULL) {
        return clk_rv_nomem;
    }

    pthread_mutex_init(&d->mutex, NULL);
    pthread_cond_init(&d->cond, NULL);

    *dh = d;

    return clk_rv_ok;
}

clk_rv_t clk_drv_linux_emit_edge (void *dh, clk_src_t *src, int set_status_bit) {
    clk_driver_dh_linux_t *d = (clk_driver_dh_linux_t *)dh;

    pthread_mutex_lock(&d->mutex);

    pthread_cond_broadcast(&d->cond);

    if (set_status_bit) {
        src->mux->status_reg |= 1 << src->id;
    }

    pthread_mutex_unlock(&d->mutex);

    return clk_rv_ok;
}


clk_rv_t clk_drv_linux_clear_status (void *dh, clk_src_t *src) {
    clk_driver_dh_linux_t *d = (clk_driver_dh_linux_t *)dh;

    pthread_mutex_lock(&d->mutex);
    src->mux->status_reg &= ~(1 << src->id);
    pthread_mutex_unlock(&d->mutex);

    return clk_rv_ok;
}


clk_rv_t clk_drv_linux_mux_point (void *dh) {
    clk_driver_dh_linux_t *d = (clk_driver_dh_linux_t *)dh;
    clk_rv_t rv = clk_rv_ok;
    int rrv;

    rrv = pthread_mutex_lock(&d->mutex);

    rrv = pthread_cond_wait(&d->cond, &d->mutex);
    if (!rrv) {
        rv = clk_rv_edge;
    }

    pthread_mutex_unlock(&d->mutex);

    return rv;
}


