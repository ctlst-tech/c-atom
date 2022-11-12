#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ebr.h"

void *ebr_alloc(size_t size) {
    return calloc(1, size);
}

unsigned load_task_clock_method(xml_node_t *task_node, swsys_task_t *task);
unsigned load_test_name_and_priority(xml_node_t *task_node, swsys_task_t *task);

#define SOURCE_TAG "source"

static unsigned count_sources_specifiers(xml_node_t *bridge_node) {

    unsigned rv = 0;

    for (xml_node_t *n = bridge_node->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, SOURCE_TAG)) {
            const char *alias = xml_attr_value(n, "alias");
            if (alias != NULL) {
                rv += xml_list_from_attr_size(alias);
            }
        }
    }

    return rv;
}

xml_rv_t func_load_connections(xml_node_t *conn_node, const char *tag, func_pair_t **spec);

static xml_rv_t load_bridge_sources(xml_node_t *bridge_node, ebr_instance_t *ebr) {
    return func_load_connections(bridge_node, "source", &ebr->sources_paths);
}

xml_rv_t ebr_load_task(xml_node_t *bridge_node, swsys_task_t *task) {
    unsigned err_num = 0;

    task->type = tsk_ebr;
    err_num += load_test_name_and_priority(bridge_node, task);
    err_num += load_task_clock_method(bridge_node, task);

    xml_rv_t xrv;

    GET_ATTR_INIT();

    if (err_num > 0) {
        return xml_e_dom_process;
    }

    ebr_instance_t *ebr = ebr_alloc(sizeof(*ebr));
    if (ebr == NULL) {
        return xml_e_nomem;
    }

    unsigned topics_in_bridge = count_sources_specifiers(bridge_node);
    eswb_rv_t erv = eswb_bridge_create(task->name, topics_in_bridge, &ebr->bridge);
    if (erv != eswb_e_ok) {
        dbg_msg("eswb_bridge_create error: %s", eswb_strerror(erv));
        err_num++;
    }

    ebr->dst_path = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, bridge_node, "to", &err_num);

    xrv = load_bridge_sources(bridge_node, ebr);
    if (xrv != xml_e_ok) {
        err_num++;
    }

    ebr_get_handler(ebr, &task->func_handler);

    return err_num > 0 ? xml_e_dom_process : xml_e_ok;
}
