#include <stdlib.h>

#include "flow.h"
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

/**
 *
 * @param flow_import_node
 * @param flow_root_path string with a size of FUNC_XML_FILEPATH_MAX_LEN
 * @return
 */
static xml_rv_t import_flow(xml_node_t *flow_import_node, char *flow_root_path) {
    int err_cnt = 0;
    GET_ATTR_INIT();

    // TODO prevent importing from circle dependances (pre register currently loading file)

    const char *import_path = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, flow_import_node, "file", &err_cnt);

    if (import_path != NULL) {
        // flow_root_path name become confusing, but let's save some stack
        strncat(flow_root_path, import_path, FUNC_XML_FILEPATH_MAX_LEN - strlen(flow_root_path));

        if (flow_load(flow_root_path, NULL) != fspec_rv_ok) {
            err_cnt++;
        }
    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}

xml_rv_t func_load_connections(xml_node_t *conn_node, const char *tag, func_pair_t **spec);

typedef struct flow_reg_record {
    const char *path;
    function_flow_t *flow;
    fspec_rv_t load_rv;

    function_handler_t handler;

    struct flow_reg_record *next;
} flow_reg_record_t;

static flow_reg_record_t *flow_reg_root = NULL;

/**
 * Warning: This call does not checks uniqueness of the flow before add it to the list
 * But duplication may happen because of recursive import
 *
 * @param flow
 * @param path
 * @param load_rv
 * @return
 */
fspec_rv_t flow_reg_add(function_flow_t *flow, const char *path, fspec_rv_t load_rv) {

    flow_reg_record_t *r;
    r = calloc(1, sizeof(*flow));
    if (r == NULL) {
        return fspec_rv_no_memory;
    }

    r->path = path;
    r->flow = flow;
    r->load_rv = load_rv;
    r->next = NULL;

    flow_get_handler(flow, &r->handler);

    if (flow_reg_root == NULL) {
        flow_reg_root = r;
    } else {
        flow_reg_record_t *n;
        for (n = flow_reg_root; n->next != NULL; n = n->next);
        n->next = r;
    }

    return fspec_rv_ok;
}

fspec_rv_t flow_reg_find_handler(const char *fname, function_handler_t **flow_rv) {
    flow_reg_record_t *n;

    for (n = flow_reg_root; n != NULL; n = n->next) {
        if (n->load_rv == fspec_rv_ok) {
            if (strcmp(n->flow->spec.name, fname) == 0) {
                *flow_rv = &n->handler;
                return fspec_rv_ok;
            }
        }
    }

    return fspec_rv_not_exists;
}

fspec_rv_t flow_reg_find_by_path(const char *path, function_flow_t **flow_rv) {
    flow_reg_record_t *n;

    for (n = flow_reg_root; n != NULL; n = n->next) {
        if (strcmp(n->path, path) == 0) {
            if (flow_rv != NULL) {
                *flow_rv = n->flow;
            }
            return n->load_rv;
        }
    }

    return fspec_rv_not_exists;
}



fspec_rv_t flow_load(const char *path, function_flow_t **flow_rv) {

    if (path == NULL) {
        return fspec_rv_invarg;
    }

    function_flow_t *flow;

    fspec_rv_t lookup_rv = flow_reg_find_by_path(path, &flow);
    if (lookup_rv != fspec_rv_not_exists) {
        // already have attempt to load, returning same error
        return lookup_rv;
    }

    fspec_rv_t rv;

    do {
        flow = flow_alloc(sizeof(*flow));


        if (flow == NULL) {
            rv = fspec_rv_no_memory;
            break;
        }

        xml_node_t *flow_xml_root;
        xml_rv_t xml_rv = xml_parse_from_file(path, &flow_xml_root);

        if (xml_rv != xml_e_ok) {
            xml_err("XML file (%s) load error", path);
            rv = fspec_rv_loaderr;
            break;
        }

        if (!xml_node_name_eq(flow_xml_root, "flow")) {
            xml_err("Invalid root tag \"%s\" for flow", flow_xml_root->name);
            rv = fspec_rv_loaderr;
            break;
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

        char *trailing_slash = strrchr(path, '/');
        char flow_root_path[FUNC_XML_FILEPATH_MAX_LEN];
        if (trailing_slash != NULL) {
            strncpy(flow_root_path, path, trailing_slash - path + 1);
        } else {
            strcpy(flow_root_path, "./");
        }

        xml_node_t *import_node = xml_node_find_child(flow_xml_root, "import");
        if (import_node != NULL) {
            for (xml_node_t *n = import_node->first_child; n != NULL; n = n->next_sibling) {
                if (xml_node_name_eq(n, "flow")) {
                    xml_rv = import_flow(n, flow_root_path);
                    if (xml_rv != xml_e_ok) {
                        err_num++;
                    }
                }
            }
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
                    xml_rv = load_func(n, (function_inside_flow_t *) &flow->cfg.functions_batch[i]);
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

        rv = err_num > 0 ? fspec_rv_loaderr : fspec_rv_ok;
    } while(0);

    if (rv == fspec_rv_ok) {
        if (flow_rv != NULL) {
            *flow_rv = flow;
        }
    }

    if (rv != fspec_rv_no_memory) {
        fspec_rv_t lrv = flow_reg_add(flow, path, rv);
        if (lrv != fspec_rv_ok) {
            return lrv;
        }
    }

    return rv;
}
