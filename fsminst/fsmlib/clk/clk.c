//
// Created by Ivan Makarov on 21/9/21.
//

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "clk.h"



clk_rv_t clk_src_add(clk_mux_t *mux, const clk_src_handlers_t *h, void *dh, clk_src_t **r) {
    if (mux->src_num >= CLOCKING_SRC_MAX) {
        return clk_rv_nomem;
    }
    clk_src_t *new = &mux->src[mux->src_num];
    new->enabled = 0;
    new->data_handle = dh;

    new->handlers = h;

    new->mux = mux;
    new->status_reg_ref = &mux->status_reg;
    new->id = mux->src_num;

    if (r) {
        *r = new;
    }

    mux->src_num++;
    return clk_rv_ok;
}

clk_rv_t clk_src_add_timer(clk_mux_t *mux, uint32_t msec, int periodic) {
    return clk_rv_ok;
}


clk_rv_t clk_src_enable(clk_src_t *s) {
    clk_rv_t rv = s->handlers->enable(s->data_handle);
    if (rv == clk_rv_ok) {
        s->enabled = 0;
    }

    return rv;
}

clk_rv_t clk_src_disable(clk_src_t *s) {
    clk_rv_t rv = s->handlers->disable(s->data_handle);
    if (rv == clk_rv_ok) {
        s->enabled = -1;
    }
    // TODO reset status?

    return rv;
}


inline static clk_rv_t clk_mux_point(clk_mux_t *m) {
    clk_src_t *src;

    if (m->status_reg) {
        return clk_rv_level;
    }

    clk_rv_t rv = m->driver->mux_point(m->driver_data_handle);
    printf("mux_point returns with %d\n", rv);


//    switch (rv) {
//        case clk_rv_edge:
//            //if (src->disabled) { // additional filter
//            //    rv = clk_rv_ok;
//            //}
//            break;
//
//        default:
//            break;
//    }

    return rv;
}

static clk_rv_t clk_mux_driver_init(clk_mux_t *m) {
    clk_rv_t rv = m->driver->init(&m->driver_data_handle);

    return rv;
}

clk_rv_t clk_mux_emit_edge(clk_src_t *src, int set_status_bit) {
    return src->mux->driver->emit_edge(src->mux->driver_data_handle, src, set_status_bit);
};

clk_rv_t clk_mux_clear_status_edge(clk_src_t *src) {
    return src->mux->driver->clear_status(src->mux->driver_data_handle, src);
}

inline static void clk_process(clk_mux_t *m) {
    m->process(m->process_data_handle);
}

_Noreturn void clk_cycle (clk_mux_t *m) {

    while (1) {
        switch (clk_mux_point(m)) {
            case clk_rv_edge:
            case clk_rv_level:
                clk_process(m);
                break;

            default:
                break;
        }
    }
}

clk_rv_t clk_init(clk_mux_t *m, clk_mux_driver_t *dr, void *data_driver, clk_process_t process, void *data_process) {
    memset(m, 0, sizeof(*m));
    m->driver = dr;
    m->driver_data_handle = data_driver;
    m->process = process;
    m->process_data_handle = data_process;
    clk_mux_driver_init(m);
    return clk_rv_ok;
}

/*
 *
 * declare clocking source
 * mask source
 * unmask source
 *
 */

/*
 * 1. must not explicitly be multithreaded
 *
 *
 */

