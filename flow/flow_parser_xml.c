//
// Created by goofy on 2/2/22.
//

#include <stdlib.h>

#include "function.h"
#include "function_xml.h"

void* flow_alloc(size_t s) {
    return calloc(1, s);
}

xml_rv_t load_func(xml_node_t *f_node, function_inside_flow_t *f) {

    int err_cnt = 0;
    GET_ATTR_INIT();

    f->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, f_node, "name", &err_cnt);
    const char *spec_name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, f_node, "by_spec", &err_cnt);

    do {
        if (spec_name == NULL) {
            err_cnt++;
            break;
        } else {
            f->h = fspec_find_handler(spec_name);
            if (f->h == NULL) {
                xml_err("Function spec \"%s\" not found", spec_name);
                break;
            }
        }

        xml_rv_t rv = func_load_inputs(f_node, (func_conn_spec_t **) &f->connect_spec);
        if ((rv != xml_e_ok) && (rv != xml_e_dom_empty)) {
            err_cnt++;
        }

        rv = func_load_params(f_node, (func_param_t **) &f->initial_params);
        if (rv != xml_e_ok && (rv != xml_e_dom_empty)) {
            err_cnt++;
        }
    } while (0);

    // TODO, check that mandatory inputs have connections


    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}

xml_rv_t func_load_connections(xml_node_t *conn_node, const char *tag, func_pair_t **spec);

fspec_rv_t flow_load(const char *path, function_flow_t **flow_rv) {

    if ((path == NULL) || (flow_rv == NULL)) {
        return fspec_rv_invarg;
    }

    function_flow_t *flow = flow_alloc(sizeof(*flow));


    xml_node_t *flow_xml_root;
    xml_rv_t xml_rv = xml_parse_from_file(path, &flow_xml_root);

    if (xml_rv != xml_e_ok) {
        xml_err("XML file (%s) load error", path);
        return fspec_rv_loaderr;
    }

    if (!xml_node_name_eq(flow_xml_root, "flow")) {
        xml_err("Invalid root tag \"%s\" for flow", flow_xml_root->name);
        return fspec_rv_loaderr;
    }

    int err_num = 0;

    GET_ATTR_INIT();

    xml_node_t *spec_node = xml_node_find_child(flow_xml_root, "spec");
    if (spec_node == NULL) {
        //xml_err("No spec is presented for fsminst \"%s\"", fsminst->name == NULL ? "N/A" : fsminst->name);
        xml_err("No spec is presented for flow");
        err_num++;
    }

    xml_rv = func_xml_load_spec(spec_node, &flow->spec);
    if (xml_rv != xml_e_ok) {
        xml_err("Spec load error for %s", path);
        err_num++;
    }

    xml_node_t *func_node = xml_node_find_child(flow_xml_root, "functions");

    int funcs_cnt = xml_node_count_siblings(func_node->first_child, "f");
    if (funcs_cnt < 1) {
        xml_err("Not a single function is presented for \"%s\"", flow->spec.name == NULL ? "N/A" : flow->spec.name);
        err_num++;
    } else {
        flow->cfg.functions_batch = flow_alloc((funcs_cnt + 1) * sizeof(function_inside_flow_t));

        int i = 0;

        for (xml_node_t *n = func_node->first_child; n != NULL; n = n->next_sibling) {
            if (xml_node_name_eq(n, "f")) {
                xml_rv = load_func(n, (function_inside_flow_t *)&flow->cfg.functions_batch[i]);
                if (xml_rv != xml_e_ok) {
                    err_num++;
                }
                i++;
            }
        }
    }

    xml_node_t *link_outputs = xml_node_find_child(flow_xml_root, "link_outputs");
    if (link_outputs == NULL) {
        xml_err("There in no output links section for %s", flow->spec.name);
        err_num++;
    } else {
#       define LINK_TAG "link"
        xml_rv = func_load_connections(link_outputs, LINK_TAG, &flow->cfg.outputs_links);
        if (xml_rv != xml_e_ok) {
            xml_err("Outputs links load failed for %s", path);
            err_num++;
        }
    }

    fspec_rv_t rv = err_num > 0 ? fspec_rv_loaderr : fspec_rv_ok;

    if (rv == fspec_rv_ok) {
        *flow_rv = flow;
    }

    return rv;

}
