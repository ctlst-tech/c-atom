#ifndef ATOMICS_CLI_CMD_H
#define ATOMICS_CLI_CMD_H

#define ATOMICS_CLI_BUS_NAME "atomics_cli"
#define ATOMICS_CLI_BUS_SPECIFIER "itb:/" ATOMICS_CLI_BUS_NAME

#define ATOMICS_CLI_MAX_COMMANDS 10
#include "eswb/errors.h"
#include "eswb/topic_proclaiming_tree.h"
#include "eswb/types.h"


typedef struct {
    int32_t cmd_bool;
} atomic_cmd_bool_t;

typedef struct {
    int32_t cmd_i32;
} atomic_cmd_i32_t;


eswb_rv_t atomics_cli_init_and_start();

eswb_rv_t atomic_cli_register_fifo(
    const topic_tree_context_t *cntx,
    const topic_proclaiming_tree_t *root,
    eswb_topic_descr_t *td);

eswb_rv_t atomics_cli_cmd_post_parse(char *cmd_line);
eswb_rv_t atomics_cli_cmd_post(const char *alias, const char *value);

#endif //ATOMICS_CLI_CMD_H
