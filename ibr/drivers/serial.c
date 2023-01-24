#include <sys/termios.h>

#include "ibr.h"
#include "ibr_process.h"

ibr_rv_t drv_file_connect(const char *path, int *td);
ibr_rv_t drv_file_disconnect(int td);
ibr_rv_t drv_file_send(int td, void *d, int *bts);
ibr_rv_t drv_file_recv(int td, void *d, int *btr);

ibr_rv_t drv_serial_connect(const char *path, int *td) {

    int baudrate = -1;
    char dev_path[50+1];

    int srv = sscanf(path, "%50[^:]:%d", dev_path, &baudrate);
    if (srv < 1) {
        return ibr_invarg;
    }

    ibr_rv_t irv = drv_file_connect(dev_path, td);
    if (irv != ibr_ok) {
        return irv;
    }

    if (baudrate > 0) {
        struct termios termios_p;

        int rv = tcgetattr(*td, &termios_p);
        if (rv) {
            return ibr_media_err;
        }

        cfsetispeed(&termios_p, baudrate);
        cfsetospeed(&termios_p, baudrate);
        cfmakeraw(&termios_p);
        rv = tcsetattr(*td, TCSANOW, &termios_p);
        if (rv) {
            return ibr_media_err;
        }
    }

    return ibr_ok;
}

const irb_media_driver_t irb_media_driver_serial = {
        .proclaim = NULL,
        .connect = drv_serial_connect,
        .disconnect = drv_file_disconnect,
        .send = drv_file_send,
        .recv = drv_file_recv,
};
