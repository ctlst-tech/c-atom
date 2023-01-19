#ifndef C_ATOM_IBR_H
#define C_ATOM_IBR_H

#include "function.h"

typedef enum {
    ibr_ok = 0,
    ibr_nomem,
    ibr_invarg,
    ibr_exist,
    ibr_noent,
    ibr_ambig,
    ibr_loaderr,
    ibr_nomedia,
    ibr_media_err,
    ibr_nomatched,
    ibr_processerr,
    ibr_not_sup,
} ibr_rv_t;

typedef struct msg msg_t;
typedef struct frame frame_t;

typedef struct protocol {
    const char *name;
    frame_t *frame;
    msg_t **msgs;
} protocol_t;

typedef struct {
    const char  *name;
    const char  *msg;
    const char  *frame;
    const char  *src;
    const char  *dst;
} process_cfg_t;


typedef struct {
    function_spec_t spec;
    protocol_t      protocol;
    process_cfg_t   *processes;
    int             processes_num;
} ibr_cfg_t;

ibr_rv_t ibr_cfg_load(const char *path, ibr_cfg_t **ibr_cfg_rv);
void ibr_get_handler(ibr_cfg_t *ibr_cfg, function_handler_t *fh);

#endif //C_ATOM_IBR_H
