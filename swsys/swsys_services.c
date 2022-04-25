#include <stdlib.h>
#include <eswb/eqrb.h>

#include "swsys.h"

swsys_rv_t swsys_service_start(const swsys_service_t *s) {
    if (strcmp(s->type, "eqrb_tcp") == 0) {
        uint16_t port;
        const char *p = fspec_find_param(s->params, "port");
        if (p != NULL) {
            port = strtoul(p, NULL, 0);
        } else {
            port = 0;
        }
        eqrb_rv_t rv = eqrb_tcp_server_start(port);
        return rv == eqrb_rv_ok ? swsys_e_ok : swsys_e_service_fail;
    } else if (strcmp(s->type, "eqrb_serial") == 0) {
        uint32_t baudrate;
        const char *br = fspec_find_param(s->params, "baudrate");
        const char *path = fspec_find_param(s->params, "path");
        const char *bus = fspec_find_param(s->params, "bus");
        if (br != NULL) {
            baudrate = strtoul(br, NULL, 0);
        } else {
            baudrate = 115200;
        }
        if (path == NULL || bus == NULL) {
            return swsys_e_invargs;
        }
        eqrb_rv_t rv = eqrb_serial_server_start(path, baudrate, bus);
        return rv == eqrb_rv_ok ? swsys_e_ok : swsys_e_service_fail;
    } else {
        return swsys_e_no_such_service;
    }
}
