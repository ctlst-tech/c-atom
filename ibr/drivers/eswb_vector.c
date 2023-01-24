#include <eswb/api.h>

#include "ibr.h"
#include "ibr_process.h"


ibr_rv_t drv_eswb_vector_connect(const char *path, int *td) {
    eswb_rv_t erv;
    ibr_rv_t rv;
    erv = eswb_connect(path, td);
    if (erv == eswb_e_ok) {
        uint32_t dummy;
        eswb_index_t br;
        // check that this is vector
        erv = eswb_vector_read(*td, 0, &dummy, 4, &br);
        if (erv == eswb_e_ok || erv == eswb_e_vector_inv_index ) {
            rv = ibr_ok;
        } else {
            rv = ibr_media_err;
        }
    } else {
        rv = ibr_nomedia;
    }
    return rv;
}

ibr_rv_t drv_eswb_vector_disconnect(int td) {
    eswb_rv_t rv;
    rv = eswb_disconnect (td);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_vector_send(int td, void *d, int *bts) {
    eswb_rv_t rv;
    rv = eswb_vector_write(td, 0, d, *bts, ESWB_VECTOR_WRITE_OPT_FLAG_DEFINE_END);
    if (rv == eswb_e_ok) {
        *bts = *bts;
    }
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_vector_recv(int td, void *d, int *btr) {
    eswb_rv_t rv;
    eswb_index_t br;
    rv = eswb_vector_get_update(td, 0, d, *btr, &br);
    if (rv == eswb_e_ok) {
        *btr = br;
    }
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}


const irb_media_driver_t irb_media_driver_eswb_vector = {
        .proclaim = NULL,
        .connect = drv_eswb_vector_connect,
        .disconnect = drv_eswb_vector_disconnect,
        .send = drv_eswb_vector_send,
        .recv = drv_eswb_vector_recv,
};
