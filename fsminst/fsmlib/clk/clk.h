//
// Created by Ivan Makarov on 21/9/21.
//

#ifndef HW_BRIDGE_CLK_H
#define HW_BRIDGE_CLK_H

#include "stdint.h"

typedef enum {
    clk_rv_ok = 0,
    clk_rv_edge = 1,
    clk_rv_level = 2,
    clk_rv_nomem
} clk_rv_t;

typedef enum clk_mux_timer_mode {
    clk_mux_timer_once,
    clk_mux_timer_periodic
} clk_mux_timer_mode_t;


typedef clk_rv_t (*clk_src_init_t)(void *);
typedef clk_rv_t (*clk_src_disable_t)(void *);
typedef clk_rv_t (*clk_src_enable_t)(void *);
typedef clk_rv_t (*clk_src_status_t)(void *);

typedef struct clk_src_handlers {
    clk_src_init_t     init;
    clk_src_disable_t  disable;
    clk_src_enable_t   enable;
    clk_src_status_t   status;
} clk_src_handlers_t;


typedef struct clk_src {
    struct clk_mux     *mux;
    const clk_src_handlers_t *handlers;
    int                enabled;
    void               *data_handle;
    uint32_t           *status_reg_ref;
    int                id;
} clk_src_t;

typedef clk_rv_t (*clk_mux_driver_init_call_t)(void **);
typedef clk_rv_t (*clk_mux_driver_mux_point_call_t)(void *);
typedef clk_rv_t (*clk_mux_driver_emit_edge_call_t)(void *, clk_src_t *, int set_status_bit);
typedef clk_rv_t (*clk_mux_driver_clear_status_call_t)(void *, clk_src_t *);
typedef clk_rv_t (*clk_mux_driver_start_timer_t)(void *, clk_src_t *, clk_mux_timer_mode_t type, uint32_t interval_ms);
typedef clk_rv_t (*clk_mux_driver_stop_timer_t)(void *, clk_src_t *);

typedef struct clk_mux_driver {
    clk_mux_driver_init_call_t init;
    clk_mux_driver_mux_point_call_t mux_point;
    clk_mux_driver_emit_edge_call_t emit_edge;
    clk_mux_driver_clear_status_call_t clear_status;
    clk_mux_driver_start_timer_t start_timer;
    clk_mux_driver_stop_timer_t stop_timer;
} clk_mux_driver_t;

typedef int (*clk_process_t)(void *);

#define CLOCKING_SRC_MAX 5

typedef struct clk_mux {
    clk_src_t src [CLOCKING_SRC_MAX];
    int src_num;

    clk_mux_driver_t *driver;
    void *driver_data_handle;

    clk_process_t process;
    void *process_data_handle;

    uint32_t      status_reg;
} clk_mux_t;



clk_rv_t clk_init(clk_mux_t *m, clk_mux_driver_t *dr, void *data_driver, clk_process_t process, void *data_process);

void clk_cycle (clk_mux_t *m);
clk_rv_t clk_src_enable(clk_src_t *s);
clk_rv_t clk_src_disable(clk_src_t *s);
clk_rv_t clk_src_add(clk_mux_t *mux, const clk_src_handlers_t *h, void *dh, clk_src_t **r);
clk_rv_t clk_mux_emit_edge(clk_src_t *src, int set_status_bit);
clk_rv_t clk_mux_clear_status_edge(clk_src_t *src);


#endif //HW_BRIDGE_CLK_H
