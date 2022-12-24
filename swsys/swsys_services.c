#include <stdlib.h>
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

#include "swsys.h"

#define SDTL_BUS_NAME "sdtl"
#define SDTL_BUS_SPECIFIER "itb:/" SDTL_BUS_NAME

static int sdtl_bus_inited = 0;

static int count_ch_resources(const swsys_service_t *s) {
    int rv = 0;

    for(int i = 0; s->resources[i].name != NULL; i++) {
        if (strcmp(s->resources[i].name, "channel") == 0) {
            rv++;
        }
    }

    return rv;
}

const char *resourse_value_attr(swsys_service_resource_t *resources, const char *resource_name) {
    for(int i = 0; resources[i].name != NULL; i++) {
        if (strcmp(resources[i].name, resource_name) == 0) {
            return resources[i].params->value; // first tag value
        }
    }

    return NULL;
}


static swsys_rv_t sdtl_service_load_channels(const swsys_service_t *s, sdtl_service_t *sdtl) {

    int i;

    sdtl_channel_cfg_t cfg;

    for(i = 0; s->resources[i].name != NULL; i++) {
        if (strcmp(s->resources[i].name, "channel") != 0) {
            continue;
        }

        memset(&cfg, 0, sizeof(cfg));

        const char *ch_id_s = fspec_find_param(s->resources[i].params, "id");
        const char *ch_type_s = fspec_find_param(s->resources[i].params, "type");
        const char *ch_mtu_s = fspec_find_param(s->resources[i].params, "mtu_override");

        cfg.name = fspec_find_param(s->resources[i].params, "name");

        if (ch_id_s == NULL || cfg.name == NULL || ch_type_s == NULL) {
            return swsys_e_invargs;
        }

        cfg.id = strtoul(ch_id_s, NULL, 0);

        if (strcmp(ch_type_s, "rel") == 0) {
            cfg.type = SDTL_CHANNEL_RELIABLE;
        } else if (strcmp(ch_type_s, "unrel") == 0) {
            cfg.type = SDTL_CHANNEL_UNRELIABLE;
        } else {
            return swsys_e_invargs;
        }

        cfg.mtu_override = ch_mtu_s != NULL ? strtoul(ch_mtu_s, NULL, 0) : 0;

        sdtl_rv_t srv;

        srv = sdtl_channel_create(sdtl, &cfg);
        if (srv != SDTL_OK) {
            return swsys_e_invargs;
        }
    }

    return swsys_e_ok;
}

static swsys_rv_t sdtl_init_and_start(const swsys_service_t *s) {

    sdtl_service_t *sdtl_service;
    sdtl_media_serial_params_t ser_params;
    uint32_t mtu;
    const char *br = resourse_value_attr(s->resources, "baudrate");
    const char *ser_path = resourse_value_attr(s->resources, "serial_port");
    const char *mtu_str = resourse_value_attr(s->resources, "mtu");
    if (br != NULL) {
        ser_params.baudrate = strtoul(br, NULL, 0);
    } else {
        ser_params.baudrate = 115200;
    }
    if (mtu_str != NULL) {
        mtu = strtoul(mtu_str, NULL, 0);
    } else {
        mtu = 0;
    }
    if (ser_path == NULL) {
        return swsys_e_invargs;
    }

    size_t ch_num = count_ch_resources(s);

    if (ch_num == 0) {
        return swsys_e_invargs;
    }

    if (!sdtl_bus_inited) {
        // TODO get proper metrics for needed topics num
        eswb_rv_t erv = eswb_create(SDTL_BUS_NAME, eswb_inter_thread, 10 + 16 * ch_num);
        if (erv != eswb_e_ok) {
            return swsys_e_service_fail;
        }

        sdtl_bus_inited = -1;
    }

    sdtl_rv_t rv = sdtl_service_init(&sdtl_service, s->name, SDTL_BUS_SPECIFIER, mtu, ch_num, &sdtl_media_serial);
    if (rv != SDTL_OK) {
        return swsys_e_service_fail;
    }

    swsys_rv_t srv = sdtl_service_load_channels(s, sdtl_service);
    if (srv != swsys_e_ok) {
        return srv;
    }

    rv = sdtl_service_start(sdtl_service, ser_path, &ser_params);
    if (rv != SDTL_OK) {
        return swsys_e_service_fail;
    }

    return swsys_e_ok;
}

swsys_rv_t swsys_service_start(const swsys_service_t *s) {

    if (strcmp(s->type, "sdtl") == 0) {
        return sdtl_init_and_start(s);
    } else if (strcmp(s->type, "eqrb_sdtl") == 0) {
        const char *sdtl_service_name = resourse_value_attr(s->resources, "sdtl_service");
        const char *sdtl_ch1_name = resourse_value_attr(s->resources, "channel_1_name");
        const char *sdtl_ch2_name = resourse_value_attr(s->resources, "channel_2_name");
        //const char *ch_mask_s = fspec_find_param(s->params, "ch_mask");
        //uint32_t ch_mask;
        const char *bus2replicate = resourse_value_attr(s->resources, "event_queue_source");

        if (sdtl_service_name == NULL || sdtl_ch1_name == NULL || bus2replicate == NULL) {
            return swsys_e_invargs;
        }

        const char *err_msg;
        eqrb_rv_t rv = eqrb_sdtl_server_start(
                s->name,
                sdtl_service_name,
                sdtl_ch1_name,
                sdtl_ch2_name,
                0xFFFFFFFF,
                bus2replicate,
                &err_msg);
        if (rv != eqrb_rv_ok) {
            dbg_msg("eqrb_sdtl_server_start for \"%s\" failed: %s", sdtl_service_name, err_msg);
        }

        return rv == eqrb_rv_ok ? swsys_e_ok : swsys_e_service_fail;
    } else if (strcmp(s->type, "eqrb_file") == 0) {
        const char *bus2replicate =
            resourse_value_attr(s->resources, "event_queue_source");
        const char *file_prefix =
            resourse_value_attr(s->resources, "file_prefix");
        const char *dst_dir = resourse_value_attr(s->resources, "dst_dir");

        if (bus2replicate == NULL || file_prefix == NULL || dst_dir == NULL) {
            return swsys_e_invargs;
        }

        const char *err_msg;
        eqrb_rv_t rv = eqrb_file_server_start(s->name, file_prefix, dst_dir,
                                              bus2replicate, &err_msg);

        if (rv != eqrb_rv_ok) {
            dbg_msg("eqrb_file_server_start for \"%s\" failed: %s", file_prefix,
                    err_msg);
        }

        return rv == eqrb_rv_ok ? swsys_e_ok : swsys_e_service_fail;
    } else {
        return swsys_e_no_such_service;
    }
}


//    if (strcmp(s->type, "eqrb_tcp") == 0) {
//        uint16_t port;
//        const char *p = fspec_find_param(s->params, "port");
//        if (p != NULL) {
//            port = strtoul(p, NULL, 0);
//        } else {
//            port = 0;
//        }
//        eqrb_rv_t rv = eqrb_tcp_server_start(port);
//        return rv == eqrb_rv_ok ? swsys_e_ok : swsys_e_service_fail;
//    {