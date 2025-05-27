#include "atomics_cli_cmd.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "eswb/api.h"

eswb_rv_t atomics_cli_init_and_start() {

    const eswb_rv_t erv = eswb_create(ATOMICS_CLI_BUS_NAME, eswb_inter_thread, 2 * ATOMICS_CLI_MAX_COMMANDS);

    return erv;
}


typedef struct {
    const char *alias;
    eswb_topic_descr_t td;
    topic_data_type_t type;
} alias2td_t;


alias2td_t alias2td[ATOMICS_CLI_MAX_COMMANDS + 1] = {
    {.alias = NULL}
};


eswb_rv_t get_td(const char *alias, eswb_topic_descr_t *td) {
    int i;
    for (i = 0; i < ATOMICS_CLI_MAX_COMMANDS && alias2td[i].alias != NULL; i++) {
        if (strcmp(alias, alias2td[i].alias) == 0) {
            *td =  alias2td[i].td;
            return eswb_e_ok;
        }
    }

    if (i >= ATOMICS_CLI_MAX_COMMANDS) {
        return eswb_e_mem_topic_max;
    }

    char cli_cmd_path[ESWB_TOPIC_MAX_PATH_LEN];
    strcpy(cli_cmd_path, ATOMICS_CLI_BUS_SPECIFIER);
    strcat(cli_cmd_path, "/");
    strcat(cli_cmd_path, alias);
    eswb_topic_descr_t ttd;
    eswb_rv_t erv = eswb_connect(cli_cmd_path, &ttd);
    if (erv != eswb_e_ok) {
        return erv;
    }

    alias2td[i].alias = strdup(alias);
    alias2td[i].td =  ttd;
    alias2td[i + 1].alias = NULL;

    *td = ttd;

    return eswb_e_ok;
}


eswb_rv_t atomics_cli_cmd_post_bool(eswb_topic_descr_t td, int32_t flag) {
    atomic_cmd_bool_t cmd;
    cmd.cmd_bool = flag;

    return eswb_fifo_push(td, &cmd);
}


eswb_rv_t atomics_cli_cmd_post_int32(eswb_topic_descr_t td, int32_t val) {

    atomic_cmd_i32_t cmd;
    cmd.cmd_i32 = val;

    return eswb_fifo_push(td, &cmd);
}


eswb_rv_t atomics_cli_cmd_post(const char *alias, const char *value) {
    eswb_topic_descr_t td;

    eswb_rv_t rv = get_td(alias, &td);

    if (rv != eswb_e_ok) {
        return rv;
    }
    // TODO validate expected type

    if (strcmp(value, "true") == 0 || strcmp(value, "set") == 0) {
        return atomics_cli_cmd_post_bool(td, 1);
    } else if (strcmp(value, "false") == 0 || strcmp(value, "clear") == 0 || strcmp(value, "unset") == 0) {
        return atomics_cli_cmd_post_bool(td, 0);
    } else {
        char *endptr;
        errno = 0;
        long int parsed_value = strtol(value, &endptr, 10);

        if (errno != 0 || *endptr != '\0' || parsed_value > INT32_MAX ||
            parsed_value < INT32_MIN) {
            return eswb_e_invargs;
        }

        return atomics_cli_cmd_post_int32(td, (int32_t)parsed_value);
    }
}


eswb_rv_t atomics_cli_cmd_post_parse(char *cmd_line) {
    // Skip leading whitespace
    while (*cmd_line == ' ') {
        cmd_line++;
    }

    char *alias = strtok(cmd_line, " ");
    // Skip any additional spaces between alias and value
    char *value = strtok(NULL, "");
    while (value != NULL && *value == ' ') {
        value++;
    }

    if (alias == NULL || value == NULL || *value == '\0') {
        return eswb_e_invargs;
    }

    return atomics_cli_cmd_post(alias, value);
}


eswb_rv_t atomic_cli_register_fifo(
    const topic_tree_context_t *cntx,
    const topic_proclaiming_tree_t *root,
    eswb_topic_descr_t *td) {

    // char path[ESWB_TOPIC_MAX_PATH_LEN];
    // strcpy(path, ATOMICS_CLI_BUS_SPECIFIER);
    // strcat(path, "/");
    // strcat(path, p->alias);

    eswb_rv_t erv = eswb_proclaim_tree_by_path(ATOMICS_CLI_BUS_SPECIFIER, root, cntx->t_num, td);

    return erv;
}
