#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Chrysalis Script Interpreter Entry Point
 * Acts as /bin/cs
 * Reads scripts exclusively from Disk (FAT32).
 */
int cmd_cs_main(int argc, char** argv);

#ifdef __cplusplus
}
#endif