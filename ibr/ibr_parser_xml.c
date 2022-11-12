#include <string.h>
#include <stdlib.h>
#include "ibr_msg.h"
#include "ibr_convert.h"

#include "xml.h"
#include "function_xml.h"

void *ibr_alloc(size_t s) {
    return calloc(1, s);
}

static xml_rv_t load_rule(xml_node_t *rule_node, field_t *field) {

    int err_cnt = 0;
    GET_ATTR_INIT();

    const char *action = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, rule_node, "action", &err_cnt);
    const char *bus_name = GET_ATTR_OPTIONAL(xml_attr_str, rule_node, "bus_name", &err_cnt);
    const char *bus_type = GET_ATTR_OPTIONAL(xml_attr_str, rule_node, "bus_type", &err_cnt);

    if (err_cnt == 0) {
        field->rule = ibr_alloc(sizeof(*field->rule));

        // TODO process rules here
    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}


static xml_rv_t load_field_array(xml_node_t *field_node, int *offset, msg_t *to_msg) {
    int err_cnt;
    GET_ATTR_INIT();

    const char *name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, field_node, "name", &err_cnt);
    const char *type_str = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, field_node, "elem_type", &err_cnt);
    int arr_size = GET_ATTR_AND_PROCESS_ERR(xml_attr_int, field_node, "size", &err_cnt);

    field_t *f;
    field_scalar_type_t elem_type = ibr_scalar_typefromstr(type_str);
    if (elem_type == ft_invalid) {
        xml_err("elem type field \"%s\" is unknown", type_str);
        err_cnt++;
    } else {
        if (elem_type == ft_char) {
            ibr_rv_t rv = ibr_add_string(to_msg, name, arr_size, offset, &f);
            if (rv != ibr_ok) {
                xml_err("ibr_add_string error (%d). Not a scalar?", rv);
                err_cnt++;
            }
        } else {
            int es = ibr_get_scalar_size(elem_type);
            ibr_rv_t rv = ibr_add_array(to_msg, name, es, arr_size, offset, &f);
            if (rv != ibr_ok) {
                xml_err("ibr_add_scalar error (%d). Not a scalar?", rv);
                err_cnt++;
            }
        }
    }


    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}


static xml_rv_t load_field_scalar(xml_node_t *field_node, int *offset, msg_t *to_msg) {
    int err_cnt;
    GET_ATTR_INIT();

    const char *name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, field_node, "name", &err_cnt);
    const char *type_str = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, field_node, "type", &err_cnt);

    field_t *f;
    field_scalar_type_t type = ibr_scalar_typefromstr(type_str);
    if (type == ft_invalid) {
        xml_err("scalar field \"%s\" is unknown", type_str);
        err_cnt++;
    } else {
        ibr_rv_t rv = ibr_add_scalar(to_msg, name, type, offset, &f);
        if (rv != ibr_ok) {
            xml_err("ibr_add_scalar error (%d). Not a scalar?", rv);
            err_cnt++;
        }
    }

//    if (err_cnt == 0) {
//        xml_node_t *rule_node;
//        if ((rule_node = xml_node_find_child(field_node, "rule")) != NULL
//            || (rule_node = xml_node_find_child(field_node, "r")) != NULL) {
//            xml_rv_t rv = load_rule(rule_node, f);
//            if (rv != xml_e_ok) {
//                err_cnt++;
//            }
//        }
//    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}

static xml_rv_t load_msg(xml_node_t *msg_node, msg_t **msg_rv) {

    xml_rv_t xrv;
    int err_num;

    msg_t *msg = ibr_alloc(sizeof(*msg));
    if (msg == NULL) {
        return xml_e_nomem;
    }

    GET_ATTR_INIT();

    msg->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, msg_node, "name", &err_num);

    int offset = 0;

    for (xml_node_t *n = msg_node->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, "fs") || xml_node_name_eq(n, "field_scalar") ) {
            xrv = load_field_scalar(n, &offset, msg);
            if (xrv != xml_e_ok) {
                err_num++;
            }
        } else if (xml_node_name_eq(n, "fa") || xml_node_name_eq(n, "field_array") ) {
            xrv = load_field_array(n, &offset, msg);
            if (xrv != xml_e_ok) {
                err_num++;
            }
        }
    }

    if (err_num == 0) {
        *msg_rv = msg;
    }

    return err_num > 0 ? xml_e_dom_process : xml_e_ok;
}

static xml_rv_t load_protocol(xml_node_t *protocol_node, protocol_t *protocol) {

    GET_ATTR_INIT();
    int err_cnt;
    xml_rv_t xrv;

    protocol->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, protocol_node, "name", &err_cnt);

    int msg_cnt = xml_node_count_siblings(protocol_node->first_child, "msg");
    if (msg_cnt < 1) {
        xml_err("Not a single message is presented for \"%s\"", protocol->name == NULL ? "N/A" : protocol->name);
        err_cnt++;
    } else {
        protocol->msgs = ibr_alloc((msg_cnt + 1) * sizeof(msg_t *));
        if (protocol->msgs == NULL) {
            return xml_e_nomem;
        }

        int i = 0;

        for (xml_node_t *n = protocol_node->first_child; n != NULL; n = n->next_sibling) {
            if (xml_node_name_eq(n, "msg")) {
                xrv = load_msg(n, &protocol->msgs[i]);
                if (xrv != xml_e_ok) {
                    err_cnt++;
                }
                i++;
            }
        }
        protocol->msgs[i] = NULL;
    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}



xml_rv_t load_process(xml_node_t *process_node, const ibr_cfg_t *ibr, process_cfg_t *process) {
    int err_cnt = 0;

    GET_ATTR_INIT();

    process->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, process_node, "name", &err_cnt);
    process->msg = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, process_node, "msg", &err_cnt);
    process->src = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, process_node, "src", &err_cnt);
    process->dst = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, process_node, "dst", &err_cnt);

//    if (process->msg == NULL) {
//        xml_err("Message \"%s\" does not specified in IBR \"%s\"", msg, ibr->spec.name != NULL ? ibr->spec.name : "N/A");
//        err_cnt++;
//    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}

ibr_rv_t ibr_cfg_load(const char *path, ibr_cfg_t **ibr_cfg_rv) {

    xml_node_t *ibr_cfg_root;
    xml_rv_t xrv = xml_parse_from_file(path, &ibr_cfg_root);
    ibr_rv_t rv;
    int err_cnt = 0;

    if (xrv != xml_e_ok) {
        xml_err("XML file (%s) load error", path);
        return ibr_loaderr;
    }

    ibr_cfg_t *ibr_cfg = ibr_alloc(sizeof(*ibr_cfg));
    if (ibr_cfg == NULL) {
        return ibr_nomem;
    }

    if (!xml_node_name_eq(ibr_cfg_root, "ibr")) {
        xml_err("Invalid root tag \"%s\" for ibr_spec", ibr_cfg_root->name);
        return ibr_loaderr;
    }

    xml_node_t *spec_node = xml_node_find_child(ibr_cfg_root, "spec");
    if (spec_node == NULL) {
        //xml_err("No spec is presented for fsminst \"%s\"", fsminst->name == NULL ? "N/A" : fsminst->name);
        xml_err("No spec is presented for ibr");
        err_cnt++;
    }

    xrv = func_xml_load_spec(spec_node, &ibr_cfg->spec);
    if (xrv != xml_e_ok) {
        xml_err("Spec load error for %s", path);
        err_cnt++;
    }

    xml_node_t *protocol_node = xml_node_find_child(ibr_cfg_root, "protocol");
    if (protocol_node == NULL) {
        xml_err("Tag \"protocol\" is not specified in \"%s\"", path);
        return ibr_loaderr;
    } else {
        xrv = load_protocol(protocol_node, &ibr_cfg->protocol);
        if (xrv != xml_e_ok) {
            err_cnt++;
        }
    }

    ibr_cfg->processes_num = xml_node_count_siblings(ibr_cfg_root->first_child, "process");
    if (ibr_cfg->processes_num < 1) {
        xml_err("Not a single process is registered for IBR \"%s\"", path);
        err_cnt++;
    } else {
        ibr_cfg->processes = ibr_alloc((ibr_cfg->processes_num) * sizeof(process_cfg_t));
        if (ibr_cfg->processes == NULL) {
            return ibr_nomem;
        }
        int i = 0;

        for (xml_node_t *n = ibr_cfg_root->first_child; n != NULL; n = n->next_sibling) {
            if (xml_node_name_eq(n, "process")) {
                xrv = load_process(n, ibr_cfg, &ibr_cfg->processes[i]);
                if (xrv != xml_e_ok) {
                    err_cnt++;
                }
                i++;
            }
        }
    }

    if (err_cnt == 0) {
        *ibr_cfg_rv = ibr_cfg;
    }

    return err_cnt > 0 ? ibr_loaderr : ibr_ok;
}
