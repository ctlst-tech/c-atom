#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>

#include "swsys/swsys.h"
#include "function.h"

int main(int argc, char* argv[]) {
    swsys_t sys;
    if (argc < 2) {
        fprintf(stderr, "Path to swsys config must be specified\n");
        return 1;
    }

    char cwd[PATH_MAX] = {0};
    char *swsys_cfg_root_dir;
    char *cfg_path = argv[1];

    if(cfg_path[0] != '/') {
        getcwd(cwd, sizeof(cwd));
        strcat(cwd, "/");
    }
    strcat(cwd, cfg_path);

    swsys_cfg_root_dir = dirname(cwd);

    swsys_rv_t rv = swsys_load(cfg_path, swsys_cfg_root_dir, &sys);
    if (rv != swsys_e_ok) {
        return 1;
    }

    return swsys_top_module_start(&sys);
}
