#ifndef C_ATOM_EBR_H
#define C_ATOM_EBR_H

#include <eswb/api.h>
#include <eswb/bridge.h>
#include "function.h"
#include "xml.h"
#include "swsys.h"

typedef struct ebr_instance {

    const char *dst_path;
    eswb_bridge_t *bridge;
    func_conn_spec_t *sources_paths;

} ebr_instance_t;

xml_rv_t ebr_load_task(xml_node_t *bridge_node, swsys_task_t *task);
void ebr_get_handler(ebr_instance_t *ebr, function_handler_t *fh);

#endif //C_ATOM_EBR_H
