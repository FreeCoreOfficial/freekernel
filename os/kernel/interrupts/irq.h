// kernel/interrupts/irq.h
#pragma once
#include <stdint.h>
#include "isr.h"   /* <<--- adăugăm struct registers_t definit aici */

#ifdef __cplusplus
extern "C" {
#endif

/* init IRQ subsystem (PIC already remapped) */
void irq_install(void);

/* handler type */
typedef void (*irq_handler_t)(registers_t*);

/* install/uninstall handlers for IRQs 0..15 */
void irq_install_handler(int irq, irq_handler_t handler);
void irq_uninstall_handler(int irq);

/* called from ASM stubs */
void irq_handler(registers_t* regs);

#ifdef __cplusplus
}
#endif
