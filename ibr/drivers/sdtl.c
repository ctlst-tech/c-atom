#include <eswb/services/sdtl.h>


#include "ibr.h"
#include "ibr_process.h"

#define MAX_CHH_NUM 4
static sdtl_channel_handle_t *channel_handlers[MAX_CHH_NUM];
int channel_handlers_num = 0;

ibr_rv_t drv_sdtl_connect(const char *path, int *td) {

    if (channel_handlers_num >= MAX_CHH_NUM) {
        return ibr_nomem;
    }

    char service_name[50+1];
    char channel_name[50+1];

    int prv = sscanf(path, "%50[^/]/%50s", service_name, channel_name);
    if (prv < 2) {
        return ibr_invarg;
    }

    sdtl_service_t *ss = sdtl_service_lookup(service_name);
    if (ss == NULL) {
        return ibr_nomedia;
    }

    ibr_rv_t rv;

    sdtl_rv_t srv = sdtl_channel_open(ss, channel_name, &channel_handlers[channel_handlers_num]);
    switch (srv) {
        case SDTL_OK:
            *td = channel_handlers_num;
            channel_handlers_num++;
            rv = ibr_ok;
            break;

        case SDTL_NO_CHANNEL_LOCAL:
            rv = ibr_nomedia;
            break;

        default:
            rv = ibr_media_err;
            break;
    }

    return rv;
}

ibr_rv_t drv_sdtl_disconnect(int td) {
    sdtl_rv_t srv = sdtl_channel_close(channel_handlers[td]);
    if (srv != SDTL_OK) {
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_sdtl_send(int td, void *d, int *bts) {

    sdtl_rv_t srv = sdtl_channel_send_data(channel_handlers[td], d, *bts);
    if (srv != SDTL_OK) {
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_sdtl_recv(int td, void *d, int *btr) {

    size_t br;

    sdtl_rv_t srv = sdtl_channel_recv_data(channel_handlers[td], d, *btr, &br);
    if (srv == SDTL_OK) {
        *btr = br;
    } else {
        return ibr_media_err;
    }

    return ibr_ok;
}


const irb_media_driver_t irb_media_driver_sdtl = {
    .proclaim = NULL,
    .connect = drv_sdtl_connect,
    .disconnect = drv_sdtl_disconnect,
    .send = drv_sdtl_send,
    .recv = drv_sdtl_recv,
};
