//
// Created by goofy on 9/13/21.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fsminst.h"
#include "function_xml.h"

void *fsminst_alloc(size_t s);

#define GET_ATTR(node__,attr_name__,err_cnt__) \
    const char *attr_name__ = GET_ATTR_AND_PROCESS_ERR(xml_attr_str,node__,#attr_name__,err_cnt__)

#define GET_ATTR_OPT(node__,attr_name__,err_cnt__) \
    const char *attr_name__ = GET_ATTR_OPTIONAL(xml_attr_str,node__,#attr_name__,err_cnt__)


static xml_rv_t load_fsm(xml_node_t *fsm_node, fsm_t **fsm_rv);

static xml_rv_t load_state(xml_node_t *state_node, fsm_t *fsm) {
    int err_cnt = 0;

    GET_ATTR_INIT();
    GET_ATTR(state_node, name, &err_cnt);

    state_t *s;
    fsm_rv_t rv = fsm_add_state(fsm, name, &s);
    if (rv != fsm_rv_ok) {
        return xml_e_dom_process;
    }

    xml_node_t *oent = xml_node_find_child(state_node, "on_enter");
    if (oent != NULL) {
        s->on_enter_code = oent->data;
    }

    xml_node_t *oex = xml_node_find_child(state_node, "on_exit");
    if (oex != NULL) {
        s->on_exit_code = oex->data;
    }


    int has_nested_states = xml_node_count_siblings(state_node->first_child, "state")
            + xml_node_count_siblings(state_node->first_child, "s");

    if (has_nested_states) {
        load_fsm(state_node, &s->nested_fsm);
    }

    return 0;
}


static xml_rv_t load_transition(xml_node_t *trns_node, fsm_t *fsm) {

    int err_cnt = 0;

    GET_ATTR_INIT();

    GET_ATTR_OPT(trns_node, name, &err_cnt);
    GET_ATTR(trns_node, from, &err_cnt);
    GET_ATTR(trns_node, to, &err_cnt);

    if (err_cnt > 0) {
        return xml_e_dom_process;
    }

    state_t *state_to = fsm_find_state(fsm, to);
    if (state_to == NULL) {
        xml_err("unable to find TO state \"%s\"", to);
        err_cnt++;
    }

    transition_t *t;
    xml_node_t *cond_node = xml_node_find_child(trns_node, "cond");
    if (cond_node == NULL ) cond_node = xml_node_find_child(trns_node, "c");

    const char *cond_expr = cond_node == NULL ? NULL : cond_node->data;

    fsm_rv_t rv = fsm_create_transition( name, cond_expr, state_to, &t);
    if (rv != fsm_rv_ok) {
        err_cnt++;
    } else {
        xml_node_t *action_node = xml_node_find_child(trns_node, "action");
        if (action_node != NULL) {
            t->transition_code = action_node->data;
        }
    }

    const char **from_list = NULL;
    xml_rv_t xrv = xml_list_from_attr(from, (char ***) &from_list);
    if (xrv == xml_e_invarg) {
        const char *default_list[] = {from, NULL};
        from_list = default_list;
    } else if (xrv == xml_e_nomem) {
        return xrv;
    }

    for (int i = 0; from_list[i] != NULL; i++) {
        state_t *state_from = fsm_find_state(fsm, from_list[i]);
        if (state_from == NULL) {
            xml_err("unable to find FROM state \"%s\"", from_list[i]);
            err_cnt++;
        } else {
            fsm_add_transition(state_from, t);
        }
    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}


static xml_rv_t load_fsm(xml_node_t *fsm_node, fsm_t **fsm_rv) {
    int err_cnt = 0;
    GET_ATTR_INIT();

    GET_ATTR(fsm_node, name, &err_cnt);

    fsm_t *fsm;

    fsm_rv_t rv = fsm_create(name, &fsm);
    if (rv != fsm_rv_ok) {
        return xml_e_nomem;
    }

    for (xml_node_t *n = fsm_node->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, "state") || xml_node_name_eq(n, "s")) {
            load_state(n, fsm);
        }
        if (xml_node_name_eq(n, "transition") || xml_node_name_eq(n, "t")) {
            load_transition(n, fsm);
        }
    }

    *fsm_rv = fsm;

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}

fsm_rv_t fsminst_load(const char *path, fsminst_t **fsminst_rv) {

    fsminst_t *fsminst = fsminst_alloc(sizeof(fsminst_t));

    if (fsminst == NULL) {
        return fsm_rv_nomem;
    }

    xml_node_t *fsminst_xml_root;
    xml_rv_t xml_rv = xml_parse_from_file(path, &fsminst_xml_root);
    int err_num = 0;


    if (xml_rv != xml_e_ok) {
        xml_err("XML file (%s) load error", path);
        return fsm_rv_loaderr;
    }

    if (!xml_node_name_eq(fsminst_xml_root, "fsminst")) {
        xml_err("Invalid root tag \"%s\" for fsminst", fsminst_xml_root->name);
        return fsm_rv_loaderr;
    }

    GET_ATTR_INIT();

    //fsminst->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, fsminst_xml_root, "name", &err_num);

    xml_node_t *spec_node = xml_node_find_child(fsminst_xml_root, "spec");
    if (spec_node == NULL) {
        //xml_err("No spec is presented for fsminst \"%s\"", fsminst->name == NULL ? "N/A" : fsminst->name);
        xml_err("No spec is presented for fsminst");
        err_num++;
    }


    xml_rv = func_xml_load_spec(spec_node, &fsminst->spec);
    if (xml_rv != xml_e_ok) {
        err_num++;
    }

    xml_node_t *vars_node = xml_node_find_child(fsminst_xml_root, "vars");
    if (vars_node != NULL) {
        xml_rv = func_xml_load_connections(vars_node, "v", (connection_spec_t ***) &fsminst->spec.state_vars);
        if (xml_rv != xml_e_ok) {
            err_num++;
        }
    }

    int fsms_cnt = xml_node_count_siblings(fsminst_xml_root->first_child, "fsm");
    if (fsms_cnt < 1) {
        xml_err("Not a single fsm is presented for \"%s\"", fsminst->spec.name == NULL ? "N/A" : fsminst->spec.name);
        err_num++;
    } else {

        fsminst->fsms = fsminst_alloc((fsms_cnt + 1) * sizeof(fsm_t *));

        int i = 0;

        for (xml_node_t *n = fsminst_xml_root->first_child; n != NULL; n = n->next_sibling) {
            if (xml_node_name_eq(n, "fsm")) {
                xml_rv = load_fsm(n, &fsminst->fsms[i]);
                if (xml_rv != xml_e_ok) {
                    err_num++;
                }
                i++;
            }
        }

        *fsminst_rv = fsminst;
    }

    return err_num > 0 ? fsm_rv_loaderr : fsm_rv_ok;
}
