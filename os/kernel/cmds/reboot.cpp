#include "reboot.h"

extern "C" void cmd_reboot(const char*) {
    asm volatile("int $0x19");
}
