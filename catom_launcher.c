#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>

#include "swsys/swsys.h"
#include "function.h"

#define CONFIG_FLAG_OPT "--config_flag="
#define CONFIG_FLAG_OPT_LEN (sizeof(CONFIG_FLAG_OPT) - 1)

static void parse_opts(int argc, char *argv[], char **config_flag) {
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], CONFIG_FLAG_OPT, CONFIG_FLAG_OPT_LEN) == 0) {
            *config_flag = argv[i] + CONFIG_FLAG_OPT_LEN;
            break;
        }
    }
}


int main(int argc, char* argv[]) {
    swsys_t sys;
    if (argc < 2) {
        fprintf(stderr, "Path to swsys config must be specified\n");
        return 1;
    }

    char cwd[PATH_MAX] = {0};
    char *swsys_cfg_root_dir;
    char *cfg_path = argv[1];
    char *config_flag = NULL;

    parse_opts(argc, argv, &config_flag);

    if(cfg_path[0] != '/') {
        getcwd(cwd, sizeof(cwd));
        strcat(cwd, "/");
    }
    strcat(cwd, cfg_path);

    swsys_cfg_root_dir = dirname(cwd);

    swsys_rv_t rv = swsys_load(cfg_path, swsys_cfg_root_dir, &sys, config_flag);
    if (rv != swsys_e_ok) {
        return 1;
    }

    return swsys_top_module_start(&sys);
}
