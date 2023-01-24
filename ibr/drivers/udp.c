#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "ibr.h"
#include "ibr_process.h"

#ifndef CATOM_NO_SOCKET

ibr_rv_t drv_udp_connect(const char *addr_str, int *md) {

    // TODO parse path, get the idea what address we opening port, receive or send

    int rc;

    int sktd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sktd == -1) {
        fprintf ( stderr, ". socket: %s", strerror ( errno ) );
        return -1;
    }

    setsockopt(sktd, SOL_SOCKET, SO_REUSEPORT, &rc, sizeof( rc ) );
    setsockopt(sktd, SOL_SOCKET, SO_REUSEADDR, &rc, sizeof( rc ) );

    int server;
    int port;
    char host[50+1];
    struct sockaddr_in addr;
    socklen_t addrlen;

    int rv = sscanf(addr_str, "%50[^:]:%d", host, &port);
    if (rv < 2) {
        return ibr_invarg;
    }

    if (strcmp(host, "*") == 0) {
        server = -1;
    } else {
        server = 0;
    }

    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addrlen = sizeof(addr);

    if (server) {
        addr.sin_addr.s_addr = INADDR_ANY;

        if ( bind(sktd, (const struct sockaddr *)&addr, addrlen) < 0 ) {
            return ibr_media_err;
        }
    } else {
        inet_aton(host, &addr.sin_addr);

        if (connect(sktd, (struct sockaddr *) &addr, addrlen) != 0) {
            return ibr_media_err;
        }
    }

    *md = sktd;

    return ibr_ok;
}

ibr_rv_t drv_udp_disconnect(int sktd) {
    int rv = close(sktd);

    return rv ? ibr_media_err : ibr_ok;
}

ibr_rv_t drv_udp_send(int sktd, void *d, int *bts) {
    int bs;

    bs = (int) send(sktd, d, *bts, 0);

    *bts = bs;

    if (bs == -1) {
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_udp_recv(int sktd, void *d, int *btr) {
    int br;

    br = (int) recv(sktd, d, *btr, 0);

    if (br == -1) {
        return ibr_media_err;
    } else if (br != *btr) {
        // TODO allow no matching by a flag? Strings as a command may have a vary lng
//        return ibr_nomatched;
    }

    *btr = br;

    return ibr_ok;
}


const irb_media_driver_t irb_media_driver_udp = {
    .proclaim = NULL,
    .connect = drv_udp_connect,
    .disconnect = drv_udp_disconnect,
    .send = drv_udp_send,
    .recv = drv_udp_recv,
};

#endif
