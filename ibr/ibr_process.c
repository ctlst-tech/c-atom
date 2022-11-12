#include <stdint.h>
#include <stdlib.h>
#include <eswb/api.h>
#include <eswb/bridge.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>


#include "ibr.h"
#include "ibr_process.h"
#include "ibr_convert.h"

void *ibr_alloc(size_t s);

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


const irb_media_driver_t drv_udp_media_driver = {
    .proclaim = NULL,
    .connect = drv_udp_connect,
    .disconnect = drv_udp_disconnect,
    .send = drv_udp_send,
    .recv = drv_udp_recv,
};

#endif

topic_data_type_t ibr_eswb_field_type(field_type_t *ft) {

    switch (ft->cls) {
        case fc_scalar:
            switch (ft->st) {
                default:
                case ft_invalid:    return tt_none;
                case ft_float:      return tt_float;
                case ft_double:     return tt_double;
                case ft_uint8:      return tt_uint8;
                case ft_int8:       return tt_int8;
                case ft_uint16:     return tt_uint16;
                case ft_int16:      return tt_int16;
                case ft_uint32:     return tt_uint32;
                case ft_int32:      return tt_int32;
                case ft_uint64:     return tt_uint64;
                case ft_int64:      return tt_int64;
            }

        case fc_array:              return tt_string;
        default:                    return tt_none;
    }
}

field_type_t ibr_field_type_from_eswb(topic_data_type_t d) {
    field_type_t rv;
    switch (d) {
        default:
        case tt_none:       rv.cls = fc_scalar; rv.st = ft_invalid; break;
        case tt_float:      rv.cls = fc_scalar; rv.st = ft_float; break;
        case tt_double:     rv.cls = fc_scalar; rv.st = ft_double; break;
        case tt_uint8:      rv.cls = fc_scalar; rv.st = ft_uint8; break;
        case tt_int8:       rv.cls = fc_scalar; rv.st = ft_int8; break;
        case tt_uint16:     rv.cls = fc_scalar; rv.st = ft_uint16; break;
        case tt_int16:      rv.cls = fc_scalar; rv.st = ft_int16; break;
        case tt_uint32:     rv.cls = fc_scalar; rv.st = ft_uint32; break;
        case tt_int32:      rv.cls = fc_scalar; rv.st = ft_int32; break;
        case tt_uint64:     rv.cls = fc_scalar; rv.st = ft_uint64; break;
        case tt_int64:      rv.cls = fc_scalar; rv.st = ft_int64; break;

        case tt_string:     rv.cls = fc_array; rv.st = ft_uint8; break;
    }

    return rv;
}

static eswb_rv_t proclaim_msg(msg_t *m, const char *dst_path, eswb_topic_descr_t *td) {

    char path[ESWB_TOPIC_MAX_PATH_LEN + 1];
    char struct_topic_name[ESWB_TOPIC_NAME_MAX_LEN + 1];

    eswb_rv_t rv = eswb_path_split(dst_path, path, struct_topic_name);

    if (rv != eswb_e_ok) {
        return rv;
    }

    int fields_num = ibr_msg_fields_num(m);
    if (fields_num == 0) {
        dbg_msg("Message has no fields");
        return eswb_e_invargs;
    }

    TOPIC_TREE_CONTEXT_LOCAL_DEFINE(cntx, fields_num + 1);

    topic_proclaiming_tree_t *rt = usr_topic_set_root(cntx, struct_topic_name, tt_struct, m->size);
    eswb_size_t offset = 0;

    // prepare topic struct
    for (field_t *n = m->fields_list_head; n != NULL; n = n->next) {
//        if (n->cls != fc_scalar) {
//            dbg_msg("Non scalar type is not supported for IBR");
//            return eswb_e_invargs;
//        }
        field_type_t ft;
        ft.cls = n->cls;
        ft.st = n->nested.scalar_type;

        topic_data_type_t eswb_data_type = ibr_eswb_field_type(&ft);
        if (eswb_data_type == tt_none) {
            dbg_msg("Non supported type (%d)", n->nested.scalar_type);
            return eswb_e_invargs;
        }

        usr_topic_add_child(cntx, rt,
                            n->name,
                            eswb_data_type,
                            n->offset,
                            n->size, TOPIC_FLAG_MAPPED_TO_PARENT);
    }

    rv = eswb_proclaim_tree_by_path(path, rt, cntx->t_num, td);
    if (rv != eswb_e_ok) {
        dbg_msg("Proclaiming error: %s", eswb_strerror(rv));
    }

    return rv;
}

ibr_rv_t drv_eswb_proclaim(const char *path, msg_t *src_msg, msg_t *dst_msg, int *td) {
    eswb_rv_t rv;

    rv = proclaim_msg(dst_msg, path, td);
    if (rv != eswb_e_ok) {
        dbg_msg("proclaim_msg failed: %s", eswb_strerror(rv));
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_eswb_connect(const char *path, int *td) {
    eswb_rv_t rv;
    rv = eswb_connect(path, td);
    return rv == eswb_e_ok ? ibr_ok : ibr_nomedia;
}

ibr_rv_t drv_eswb_disconnect(int td) {
    eswb_rv_t rv;
    rv = eswb_disconnect (td);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_send(int td, void *d, int *bts) {
    eswb_rv_t rv;
    rv = eswb_update_topic(td, d);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_recv(int td, void *d, int *btr) {
    eswb_rv_t rv;
    rv = eswb_get_update(td, d);
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}


const irb_media_driver_t drv_eswb_media_driver = {
        .proclaim = drv_eswb_proclaim,
        .connect = drv_eswb_connect,
        .disconnect = drv_eswb_disconnect,
        .send = drv_eswb_send,
        .recv = drv_eswb_recv,
};

#define MAX_BRIDGES 5
static int bridges_num = 0;
static eswb_bridge_t *bridges[MAX_BRIDGES];

static eswb_rv_t create_bridge(const char *path, msg_t *dst_msg, msg_t *src_msg, eswb_topic_descr_t *td) {


    if (!bridges_num) {
        memset(bridges, 0, sizeof(bridges));
    }

    char tpath [ESWB_TOPIC_MAX_PATH_LEN + 1];
    strncpy(tpath, path, ESWB_TOPIC_MAX_PATH_LEN - 1);
    strcat(tpath, "/");
    size_t path_len = strlen(tpath);

    if (bridges_num >= MAX_BRIDGES) {
        return eswb_e_mem_data_na;
    }

    if (ibr_msg_fields_num(src_msg) != 0) {
        return eswb_e_invargs;
    }

    eswb_bridge_t *br = NULL;
    eswb_rv_t rv = eswb_bridge_create(src_msg->name,
                                      ibr_msg_fields_num(dst_msg),
                                      &br);
    if (rv != eswb_e_ok) {
        return rv;
    }

    int offset = 0;

    for (field_t *f = dst_msg->fields_list_head; f != NULL; f = f->next) {
        tpath[path_len] = 0;
        strncat(tpath, f->name, ESWB_TOPIC_MAX_PATH_LEN - path_len);
        ibr_rv_t irv;

        rv = eswb_bridge_add_topic(br, 0, tpath, f->name);
        if (rv == eswb_e_ok) {
            field_type_t ft = ibr_field_type_from_eswb(br->topics[br->tds_num-1].type); // nasty: last added topic, from prev call
            int field_size = (int) br->topics[br->tds_num-1].size;
            switch (ft.cls) {
                case fc_scalar:
                    irv = ibr_add_scalar(src_msg, f->name,
                                         ft.st,
                                         &offset, NULL);
                    break;

                case fc_array:
                    irv = ibr_add_array(src_msg, f->name,
                                        ibr_get_scalar_size(ft.st), field_size,
                                         &offset, NULL);
                    break;

                default:
                    irv = ibr_invarg;
                    break;
            }
        } else if (rv == eswb_e_no_topic) {
            irv = ibr_add_dummy(src_msg, f->name, f->size, &offset, NULL);
        } else {
            return rv;
        }

        if (irv != ibr_ok) {
            return eswb_e_invargs;
        }
    }

    if (br->tds_num == 0) {
        return eswb_e_no_topic;
    }

    *td = bridges_num;
    bridges[bridges_num] = br;
    bridges_num++;

    return eswb_e_ok;
}

ibr_rv_t drv_eswb_bridge_proclaim(const char *path, msg_t *src_msg, msg_t *dst_msg, int *td) {
    eswb_rv_t rv;

    rv = create_bridge(path, dst_msg, src_msg, td);
    if (rv != eswb_e_ok) {
        dbg_msg("create_bridge failed: %s", eswb_strerror(rv));
        return ibr_media_err;
    }

    return ibr_ok;
}

ibr_rv_t drv_eswb_bridge_disconnect(int td) {
    eswb_rv_t rv = eswb_e_ok;
    // TODO
    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}

ibr_rv_t drv_eswb_bridge_recv(int td, void *d, int *btr) {
    eswb_rv_t rv;
    eswb_bridge_t *br = bridges[td];

    // clocking using last topic
    rv = eswb_get_update(br->topics[br->tds_num-1].td, NULL);
    if (rv == eswb_e_ok) {
        rv = eswb_bridge_read(bridges[td], d);
    }

    *btr = (int)br->buffer2post_size;

    return rv == eswb_e_ok ? ibr_ok : ibr_media_err;
}


const irb_media_driver_t drv_eswb_bridge_media_driver = {
        .proclaim = drv_eswb_bridge_proclaim,
        .connect = NULL,
        .disconnect = drv_eswb_disconnect,
        .send = NULL,
        .recv = drv_eswb_bridge_recv,
};

const irb_media_driver_t *ibr_get_driver (irb_media_driver_type_t mdt) {
    switch (mdt) {
        case mdt_function:              return &drv_eswb_media_driver;
        case mdt_function_bridge:       return &drv_eswb_bridge_media_driver;
#       ifndef CATOM_NO_SOCKET
        case mdt_udp:                   return &drv_udp_media_driver;
#       endif
        default:                        return NULL;
    }
}


ibr_rv_t ibr_decode_addr(const char *addr, irb_media_driver_type_t *mdt, const char **addr_rv) {
    char drv_alias[21] = "";
#define PATH_SEPARATOR "://"
    if (sscanf(addr, "%20[a-z_]" PATH_SEPARATOR, drv_alias) < 1) {
        return ibr_invarg;
    }

    if (strcmp(drv_alias, "func") == 0) {
        *mdt = mdt_function;
    } else if (strcmp(drv_alias, "func_br") == 0) {
        *mdt = mdt_function_bridge;
    } else if (strcmp(drv_alias, "udp") == 0) {
        *mdt = mdt_udp;
    } else {
        return ibr_invarg;
    }

    *addr_rv = addr + strlen(drv_alias) + strlen(PATH_SEPARATOR);

    return ibr_ok;
}

void ibr_set_pthread_name(const char *tn);

static inline ibr_rv_t ibr_process (irb_process_setup_t *setup) {
    int msg_size = setup->dst_msg->size;
    const irb_media_driver_t *dev_src = setup->src.drv;
    const irb_media_driver_t *dev_dst = setup->dst.drv;

    void *src_buf = ibr_alloc(msg_size);
    void *dst_buf = ibr_alloc(msg_size);

    int srcd = setup->src.descr;
    int dstd = setup->dst.descr;

    ibr_rv_t rv;

    conv_instr_queue_t *conv_instrs = &setup->conv_instrs;

    ibr_set_pthread_name(setup->name);

    while(1) {
        int br = msg_size;
        rv = dev_src->recv(srcd, src_buf, &br);
        if (rv == ibr_ok) {

            // TODO convert format, apply rules

            conv_exec(conv_instrs, src_buf, msg_size, dst_buf, msg_size);

            int bw = msg_size;
            rv = dev_dst->send(dstd, dst_buf, &bw);
            if (rv != ibr_ok) {
                // TODO ?
                break;
            }
        }
    }

    return rv;
}

void *ibr_process_thread (void *setup) {
    ibr_process((irb_process_setup_t *) setup);
    return NULL;
}

ibr_rv_t ibr_process_start (irb_process_setup_t *setup) {
    int rv = pthread_create(&setup->tid, NULL, ibr_process_thread, setup);

    return rv ? ibr_processerr : ibr_ok;
}
