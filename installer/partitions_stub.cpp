#include "../os/kernel/cmds/disk.h"

extern "C" struct part_assign g_assigns[26] = {0};

extern "C" void disk_probe_partitions(void) {
    /* no-op for installer */
}
