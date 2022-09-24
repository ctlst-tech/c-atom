//
// Created by goofy on 2/3/22.
//

#include <stdlib.h>

#include "function.h"
#include "function_xml.h"
#include "xml.h"



void *func_alloc(size_t s) {
    return calloc(1, s);
}



#include "ibr_msg.h" // FIXME dirty reusage of type parser


topic_data_type_t get_conn_type(const char *s) {
    field_type_t ft;

    if (strcmp(s, "str") == 0 || strcmp(s, "string") == 0 ) {
        ft.cls = fc_array;
        ft.st = ft_uint8;
    } else {
        ft.cls = fc_scalar;
        ft.st = ibr_scalar_typefromstr(s);
    }
    return ibr_eswb_field_type(&ft);
}

static xml_rv_t load_connectivity_spec(xml_node_t *conn_node, const char *tag, connection_spec_t ***spec_rv){
    int specs_num = xml_node_count_siblings(conn_node->first_child, tag);
    connection_spec_t **specs = func_alloc((specs_num + 1) * sizeof(connection_spec_t));

    if (specs == NULL) {
        return xml_e_nomem;
    }

    xml_rv_t rv;
    int err_num = 0;
    int i = 0;

    GET_ATTR_INIT();

    for (xml_node_t *n = conn_node->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, tag)) {
            connection_spec_t *sp = func_alloc(sizeof(connection_spec_t));
            if (sp == NULL) {
                // TODO free specs
                return xml_e_nomem;
            }

            sp->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, n, "alias", &err_num);
            sp->annotation = GET_ATTR_OPTIONAL(xml_attr_str, n, "annotation", &err_num);
            sp->default_value = GET_ATTR_OPTIONAL(xml_attr_str, n, "default", &err_num);
            if (sp->default_value == NULL) {
                sp->flags.mandatory = 1;
            }
//            sp->flags.mandatory = GET_ATTR_OPTIONAL(xml_attr_bool, n, "mandatory", &err_num);

            const char *type_str = GET_ATTR_OPTIONAL(xml_attr_str, n, "type", &err_num);

            sp->type = type_str == NULL ? tt_double : get_conn_type(type_str);
            if (sp->type == tt_none) {
                xml_err("Invalid type specification for %s \"%s\"", tag, sp->name);
            }

            specs[i++] = sp;
        }
    }

    specs[i] = NULL;
    *spec_rv = specs;

    return (err_num > 0) ? xml_e_dom_process : xml_e_ok;
}

static int process_spec(xml_node_t* n, const char *tag, connection_spec_t ***spec) {

    xml_rv_t rv = load_connectivity_spec(n, tag, spec);

    return rv != xml_e_ok ? 1 : 0;
}

xml_rv_t func_xml_load_connections(xml_node_t* n, const char *tag, connection_spec_t ***spec) {
    return load_connectivity_spec(n, tag, spec);
}

xml_rv_t func_xml_load_spec(xml_node_t* spec_node, function_spec_t *func_spec) {

    int err_cnt = 0;

    xml_rv_t rv;

    memset(func_spec, 0, sizeof(*func_spec));

    GET_ATTR_INIT();

    func_spec->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, spec_node, "name", &err_cnt);

    xml_node_t *ins = xml_node_find_child(spec_node, "inputs");
    xml_node_t *outs = xml_node_find_child(spec_node, "outputs");
    xml_node_t *params = xml_node_find_child(spec_node, "params");

    if (ins != NULL) {
        err_cnt += process_spec(ins, "i", (connection_spec_t ***) &func_spec->inputs);
    }
    if (outs != NULL) {
        err_cnt += process_spec(outs, "o", (connection_spec_t ***) &func_spec->outputs);
    }
    if (params != NULL) {
        err_cnt += process_spec(params, "p", (connection_spec_t ***) &func_spec->params);
    }

    return err_cnt > 0 ? xml_e_dom_process : xml_e_ok;
}

static int parse_and_process_spec(char **aliases, const char *value, func_pair_t *spec) {

    int i;

    int has_macros = 0;
    size_t macros_pos;
    char macrosed_value[ESWB_TOPIC_MAX_PATH_LEN + 1];

    char *m;
    if ((m = strstr(value, "$alias")) != NULL) {
        has_macros = -1;
        macros_pos = m - value;
        strncpy(macrosed_value, value, ESWB_TOPIC_MAX_PATH_LEN);
    }

    for (i = 0; aliases[i] != NULL; i++) {
        spec[i].alias = aliases[i];
        if (has_macros) {
            macrosed_value[macros_pos] = 0;

            // FIXME macros might be only in the end for now
            strncat(macrosed_value, aliases[i], ESWB_TOPIC_MAX_PATH_LEN - macros_pos);
            spec[i].value = xml_strdup(macrosed_value);
        } else {
            spec[i].value = value;
        }
    }

    return i;
}

xml_rv_t func_load_connections(xml_node_t *conn_node, const char *tag, func_pair_t **spec) {

    int spec_num = xml_node_count_siblings(conn_node->first_child, tag);

    if (spec_num <= 0) {
//        xml_err("Load connections of node \"%s\" have zero specifications of tag \"%s\" for parent \"%s\"",
//                conn_node->name, tag, conn_node->parent->name);
//        return xml_e_dom_process;
        return xml_e_dom_empty;
    }

    xml_rv_t rv;
    int err_num = 0;

    GET_ATTR_INIT();

#define ALIAS_ATTR "alias"

    int max_aliases_list_len = 0;

    spec_num = 0;
    // calc overall spec size, because some lines may operate lists of aliases
    for (xml_node_t *n = conn_node->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, tag)) {
            const char *alias = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, n, ALIAS_ATTR, &err_num);
            if (alias != NULL) {
                int ls = xml_list_from_attr_size(alias);
                max_aliases_list_len = ls > max_aliases_list_len ? ls : max_aliases_list_len;
                spec_num += ls;
            }
        }
    }

    func_conn_spec_t *s = func_alloc((spec_num + 1) * sizeof(func_conn_spec_t));
    char **attr_list = xml_list_from_attr_alloc(max_aliases_list_len);

    if (s == NULL) {
        return xml_e_nomem;
    }
    if (attr_list == NULL) {
        return xml_e_nomem;
    }

    int i = 0;

    for (xml_node_t *n = conn_node->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, tag)) {
            const char *alias = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, n, ALIAS_ATTR, &err_num);
            const char *value = n->data;
            if ((value == NULL) || value[0] == 0) {
                xml_err("Value must be specified for \"%s\" %s", alias != NULL ? alias : "N\\A", tag);
                err_num++;
            }

            rv = xml_list_from_attr(alias, &attr_list);
            if (rv == xml_e_ok) {
                i += parse_and_process_spec(attr_list, value, &s[i]);
            } else if (rv == xml_e_invarg){
                // not a list
                // TODO check macro
                s[i].alias = alias;
                s[i].value = value;
                i++;
            }
        }
    }

    s[i].alias = NULL;
    s[i].value = NULL;

    *spec = s;

    // TODO check duplicates



    return (err_num > 0) ? xml_e_dom_process : xml_e_ok;
}



xml_rv_t func_load_inputs(xml_node_t *conn_node, func_conn_spec_t **spec) {
    return func_load_connections(conn_node, "in", spec);
}

xml_rv_t func_load_outputs(xml_node_t *conn_node, func_conn_spec_t **spec) {
    return func_load_connections(conn_node, "out", spec);
}

xml_rv_t func_load_params(xml_node_t *conn_node, func_param_t **spec) {
    return func_load_connections(conn_node, FUNC_XML_TAG_PARAM, spec);
}

xml_rv_t func_load_attrs_as_params(xml_node_t *node, func_param_t **spec) {
    int err_num = 0;

    int spec_num = xml_node_count_attrs(node);

    func_conn_spec_t *s = func_alloc((spec_num + 1) * sizeof(func_conn_spec_t));

    if (s == NULL) {
        return xml_e_nomem;
    }

    int i = 0;

    for (xml_attr_t *n = node->attrs_list; n != NULL; n = n->next_attr) {

        if ((n->value == NULL) || n->value[0] == 0) {
            xml_err("Value must be specified for \"%s\"", n->name != NULL ? n->name : "N\\A");
            err_num++;
        }

        // TODO check macro
        s[i].alias = n->name;
        s[i].value = n->value;
        i++;
    }

    s[i].alias = NULL;
    s[i].value = NULL;

    *spec = s;

    // TODO check duplicates

    return (err_num > 0) ? xml_e_dom_process : xml_e_ok;
}
