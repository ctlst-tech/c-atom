#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "ibr.h"
#include "ibr_process.h"



ibr_rv_t drv_file_connect(const char *path, int *td) {
    int fd;
    ibr_rv_t rv = ibr_ok;
    fd = open(path, O_RDWR);
    if (fd < 0) {
        switch (errno) {
            case ENOENT: rv = ibr_nomedia; break;
            default:     rv = ibr_media_err; break;
        }
    }

    if (rv == ibr_ok) {
        *td = fd;
    }

    return rv;
}

ibr_rv_t drv_file_disconnect(int td) {
    int rv = close(td);
    return rv ? ibr_media_err : ibr_ok;
}

ibr_rv_t drv_file_send(int td, void *d, int *bts) {
    int bs;

    bs = write(td, d, *bts);

    *bts = bs;

    if (bs == -1) {
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_file_recv(int td, void *d, int *btr) {
    int br;

    br = read(td, d, *btr);

    if (br == -1) {
        return ibr_media_err;
    }

    *btr = br;

    return ibr_ok;
}

const irb_media_driver_t irb_media_driver_file = {
        .proclaim = NULL,
        .connect = drv_file_connect,
        .disconnect = drv_file_disconnect,
        .send = drv_file_send,
        .recv = drv_file_recv,
};
