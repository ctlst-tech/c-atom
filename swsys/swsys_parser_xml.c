#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "swsys.h"
#include "xml.h"
#include "function_xml.h"
#include "ebr.h"

void *swsys_alloc(size_t s) {
    return calloc(1, s);
}

static xml_rv_t load_dir_attrs(xml_node_t *dir_node, swsys_bus_directory_t *dir) {
    int err_num = 0;
    GET_ATTR_INIT();
    dir->path = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, dir_node, "name", &err_num);
    dir->eq_channel = GET_ATTR_OPTIONAL(xml_attr_int, dir_node, "eq_channel", &err_num);

    return err_num > 0 ? xml_e_dom_process : xml_e_ok;
}

static xml_rv_t load_dirs(xml_node_t *parent_node, swsys_bus_directory_t **dirs_rv) {

    #define TAG_DIR "dir"

    int err_num = 0;
    swsys_bus_directory_t *dirs;

    int dirs_num = xml_node_count_siblings(parent_node->first_child, TAG_DIR);
    if (dirs_num > 0) {
        dirs = swsys_alloc((dirs_num + 1) * sizeof(swsys_bus_directory_t));
        if (dirs == NULL) {
            return xml_e_nomem;
        }

        int i = 0;
        for (xml_node_t *n = parent_node->first_child; n != NULL; n = n->next_sibling) {
            if (xml_node_name_eq(n, TAG_DIR)) {
                xml_rv_t xrv = load_dir_attrs(n, &dirs[i]);
                if (xrv == xml_e_ok) {
                    xrv = load_dirs(n, &dirs[i].dirs);
                    if (xrv != xml_e_ok) {
                        err_num++;
                    }
                    i++;
                } else {
                    err_num++;
                }
            }
        }

        *dirs_rv = dirs;
        dirs[i].path = NULL;
    } else {
        dirs = NULL;
    }



    return err_num > 0 ? xml_e_dom_process : xml_e_ok;
}

static xml_rv_t load_bus(xml_node_t *bus_node, swsys_bus_t *bus) {

    xml_rv_t rv;
    int err_num = 0;

    GET_ATTR_INIT();

    bus->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, bus_node, "name", &err_num);
    bus->max_topics = GET_ATTR_AND_PROCESS_ERR(xml_attr_int, bus_node, "max_topics", &err_num);
    bus->eq_channel = GET_ATTR_OPTIONAL(xml_attr_int, bus_node, "eq_channel", &err_num);

#define TAG_EVENT_QUEUE "event_queue"

    xml_node_t *event_queue = xml_node_find_child(bus_node, TAG_EVENT_QUEUE);
    if (event_queue != NULL) {
        bus->evq_size = GET_ATTR_AND_PROCESS_ERR(xml_attr_int, event_queue, "size", &err_num);
        bus->evq_buffer_size = GET_ATTR_AND_PROCESS_ERR(xml_attr_int, event_queue, "buffer_size", &err_num);
    }

    rv = load_dirs(bus_node, &bus->dirs);
    if (rv != xml_e_ok) {
        err_num++;
    }

    return err_num > 0 ? xml_e_dom_process : xml_e_ok;;
}

swsys_task_type_t task_type_from_str(const char *tts) {
    if (strcmp(tts, "flow") == 0) return tsk_flow;
    else if (strcmp(tts, "fsm") == 0) return tsk_fsm;
    else if (strcmp(tts, "ibr") == 0) return tsk_ibr;
    else if (strcmp(tts, "arbitrary") == 0) return tsk_arbitrary;
    else return tsk_unknown;
}

swsys_task_clk_method_t task_clk_method_from_str(const char *m) {
    if (strcmp(m, "freerun") == 0) return swsys_clk_freerun;
    else if (strcmp(m, "timer") == 0) return swsys_clk_timer;
    else if (strcmp(m, "inp_upd") == 0) return swsys_clk_inp_upd;
    else return swsys_clk_none;
}

unsigned load_task_clock_method(xml_node_t *task_node, swsys_task_t *task) {
    unsigned err_num = 0;
    GET_ATTR_INIT();

    const char *clk_method =  GET_ATTR_AND_PROCESS_ERR(xml_attr_str, task_node, "clk_method", &err_num);
    if (clk_method != NULL) {
        task->clk_method = task_clk_method_from_str(clk_method);
        if (task->clk_method == swsys_clk_none) {
            xml_err("Invalid clk_method for %s", task->name);
            err_num++;
        } else {
            switch (task->clk_method) {
                default:
                case swsys_clk_none:
                    xml_err("Invalid clk_method for %s", task->name);
                    err_num++;
                    break;

                case swsys_clk_ext_call:
                    xml_err("swsys_clk_external_call is not supported yet");
                    break;

                case swsys_clk_freerun:
                    break;

                case swsys_clk_timer:
                    task->clk_period_ms = GET_ATTR_AND_PROCESS_ERR(xml_attr_int, task_node, "clk_period", &err_num);
                    break;

                case swsys_clk_inp_upd:
                    task->clk_input_path = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, task_node, "clk_input_path", &err_num);
                    break;
            }
        }
    }

    return err_num;
}

unsigned load_test_name_and_priority(xml_node_t *task_node, swsys_task_t *task) {
    unsigned err_num = 0;

    GET_ATTR_INIT();

    task->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, task_node, "name", &err_num);
    task->priority = GET_ATTR_AND_PROCESS_ERR(xml_attr_int, task_node, "priority", &err_num);

    return err_num;
}

static xml_rv_t load_task(xml_node_t *task_node, const char *top_cfg_dir, swsys_task_t *task) {

    xml_rv_t rv;
    unsigned err_num = 0;

    GET_ATTR_INIT();

    err_num += load_test_name_and_priority(task_node, task);

    const char *cfg_path = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, task_node, "config", &err_num);

    if (top_cfg_dir != NULL && cfg_path != NULL) {
        char *abs_path = swsys_alloc(strlen(top_cfg_dir) + strlen(cfg_path) + 2);
        if (abs_path == NULL) {
            return xml_e_nomem;
        }

        strcpy(abs_path, top_cfg_dir);
        strcat(abs_path, "/");
        strcat(abs_path, cfg_path);

        task->config_path = abs_path;
    } else {
        task->config_path = cfg_path;
    }

    const char *type =  GET_ATTR_AND_PROCESS_ERR(xml_attr_str, task_node, "type", &err_num);
    if (type != NULL) {
        task->type = task_type_from_str(type);
        if (task->type == tsk_unknown) {
            xml_err("Invalid task type for %s", task->name);
            err_num++;
        }
    }

    err_num += load_task_clock_method(task_node, task);

    rv = func_load_params(task_node, &task->params);
    if (rv != xml_e_ok && rv != xml_e_dom_empty) {
        err_num++;
    }

    if (err_num == 0) {
#   define TAG_CONNECT "connect"
        xml_node_t *conn = xml_node_find_child(task_node, TAG_CONNECT);
        if (conn != NULL) {
            if (!xml_node_child_is_unique(task_node, TAG_CONNECT)) {
                xml_err("Ambitious connection specification for %s", task->name);
            }

            rv = func_load_inputs(conn, &task->conn_specs_in);
            if (rv != xml_e_ok && rv != xml_e_dom_empty) {
                xml_err("Inputs specification error connection specification for %s: %s", task->name, xml_strerror(rv));
                err_num++;
            }
            rv = func_load_outputs(conn, &task->conn_specs_out);
            if (rv != xml_e_ok && rv != xml_e_dom_empty) {
                xml_err("Output specification error connection specification for %s: %s", task->name, xml_strerror(rv));
                err_num++;
            }
        }
    }

    return err_num > 0 ? xml_e_dom_process : xml_e_ok;
}

static xml_rv_t load_service_resource(xml_node_t *resource_node, swsys_service_resource_t *resource) {
    resource->name = resource_node->name;
    xml_rv_t rv = func_load_attrs_as_params(resource_node, &resource->params);
    return rv;
}

static xml_rv_t load_service(xml_node_t *service_node, swsys_service_t *service) {

    int err_num = 0;
    GET_ATTR_INIT();

    service->name = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, service_node, "name", &err_num);
    service->type = GET_ATTR_AND_PROCESS_ERR(xml_attr_str, service_node, "type", &err_num);
    service->priority = GET_ATTR_OPTIONAL(xml_attr_int, service_node, "priority", &err_num);

    xml_rv_t rv;

    int resources_num = xml_node_count_siblings(service_node->first_child, NULL);

    service->resources = swsys_alloc(sizeof(*service->resources) * (resources_num + 1));
    int i = 0;

    for (xml_node_t *n = service_node->first_child; n != NULL; n = n->next_sibling) {
        rv = load_service_resource(n, &service->resources[i]);
        if (rv != xml_e_ok) {
            err_num++;
        }
        i++;
    }
    service->resources[i].name = NULL;

    return err_num > 0 ? xml_e_dom_process : xml_e_ok;
}


swsys_rv_t swsys_load(const char *path, const char *swsys_root_dir, swsys_t *sys) {

    xml_node_t *xml_root;
    xml_rv_t xrv = xml_parse_from_file(path, &xml_root);
    swsys_rv_t rv;

    if (xrv != xml_e_ok) {
        xml_err("XML file (%s) load error %s", path, xml_strerror(xrv));
        return swsys_e_loaderr;
    }

    memset(sys, 0, sizeof(*sys));

    if (!xml_node_name_eq(xml_root, "swsys")) {
        xml_err("Invalid root tag \"%s\" for swsys", xml_root->name);
        return swsys_e_loaderr;
    }

    for (xml_node_t *n = xml_root->first_child; n != NULL; n = n->next_sibling) {
        if (xml_node_name_eq(n, "bus")) {
            xrv = load_bus(n, &sys->busses[sys->busses_num]);
            if (xrv == xml_e_ok) {
                sys->busses_num++;
            }
        } else if (xml_node_name_eq(n, "task")) {
            xrv = load_task(n, swsys_root_dir, &sys->tasks[sys->tasks_num]);
            if (xrv == xml_e_ok) {
                sys->tasks_num++;
            }
        } else if (xml_node_name_eq(n, "bridge")) {
            // ESWB bridge service is wrapped inside usual task for simplicity
            xrv = ebr_load_task(n, &sys->tasks[sys->tasks_num]);
            if (xrv == xml_e_ok) {
                sys->tasks_num++;
            }
        } else if (xml_node_name_eq(n, "service")) {
            xrv = load_service(n, &sys->services[sys->services_num]);
            if (xrv == xml_e_ok) {
                sys->services_num++;
            } else {
                xml_err("swsys service load error: %s", xml_strerror(xrv));
            }
        }
    }

    return swsys_e_ok;
}
