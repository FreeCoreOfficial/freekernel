# Chrysalis OS – Modding Guide

Chrysalis OS is designed from the ground up to be **hackable, moddable, and fork-friendly**.
If you like to dig into kernels, drivers, UI stacks, or build your own OS flavor — this is for you.

This document explains how to safely and sanely modify Chrysalis OS.

---

## Philosophy

- Chrysalis OS is a **base system**, not a locked product.
- Modders are encouraged to:
  - Replace subsystems
  - Extend the kernel
  - Build custom UI stacks
  - Create entirely new distributions

There is **no enforced ABI stability** at this stage.
If you mod it, you own the result.

---

## License & Ownership

Chrysalis OS is released under the **MIT License**.

This means:
- You may fork, modify, redistribute, and rebrand.
- You may use it commercially or non-commercially.
- You may create closed or open derivatives.

**Requirement:**
- You must preserve the original copyright notice.
- You must mention that your project is based on Chrysalis OS.

Other than that — **full control is yours**.

---

## Kernel Modding Guidelines

### Scheduling & Context Switching

If you want real preemptive multitasking:

- Implement a robust context switch routine:
  - Save/restore full CPU state (general registers + EFLAGS).
  - A minimal `mov esp` + `ret` approach is fragile.
- Perform scheduling from a timer IRQ (PIT or HPET).
- Ensure safe stack handling for kernel and user tasks.

**Tip:**  
Create a small switch test without interrupts first, then move to IRQ-driven scheduling.

---

### Interrupt Safety

To avoid catastrophic crashes:

- Always install a **double-fault handler**.
- On double-fault:
  - Print debug info (serial preferred).
  - Halt safely instead of letting the CPU triple-fault.
- A triple-fault causes a reset and is not recoverable.

---

### Diagnostics & Debugging

Strongly recommended practices:

- Keep **serial logging enabled** in early boot.
- Log every critical subsystem init.
- Use VM snapshots before enabling experimental features.
- Validate new subsystems in isolation.

Serial logs save lives.

---

## UI / WM / FlyUI Modding

The UI stack is intentionally modular:

- `kernel/ui/compositor` – pixel composition
- `kernel/ui/wm` – window management
- `kernel/ui/flyui` – GUI toolkit

You may:
- Replace FlyUI entirely
- Write a new WM
- Build text, hybrid, or full graphical systems
- Implement theme engines or scripting layers

No UI assumptions are enforced by the kernel.

---

## Stability Expectations

- This is **not a stable ABI project**.
- Kernel interfaces may change.
- Modders are expected to maintain their forks.

This is freedom by design.

---

## Final Note

Chrysalis OS exists to be transformed.

If you break it — you learned something.  
If you improve it — share it.  
If you fork it — own it.

Happy hacking.
