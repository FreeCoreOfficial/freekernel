# Chrysalis OS – Programming Languages

This document lists the programming languages used in the development of
**Chrysalis OS**, along with their roles and design rationale.

Chrysalis OS intentionally uses a **small, explicit language set**
to keep the system understandable, debuggable, and modder-friendly.

---

## Primary Languages

### C

**Status:** Core language  
**Usage:** Kernel, drivers, subsystems, core libraries

C is the primary language of Chrysalis OS.

It is used for:
- Kernel core logic
- Memory management (PMM, VMM)
- Device drivers (PS/2, GPU, AHCI, USB, etc.)
- Filesystems
- ACPI, APIC, SMP, HPET
- UI subsystems (compositor, window manager, FlyUI core)

**Why C:**
- Predictable memory layout
- Full control over hardware
- No runtime dependencies
- Easy to interface with assembly
- Ideal for freestanding environments

---

### C++

**Status:** Selective usage  
**Usage:** Higher-level kernel subsystems and UI logic

C++ is used in a **restricted, kernel-safe subset**.

Used mainly for:
- UI systems (FlyUI)
- Structured managers (window manager, registries)
- Safer abstractions (RAII-style patterns where appropriate)

**Restrictions:**
- No exceptions
- No RTTI
- No standard library
- No dynamic runtime features

C++ is treated as “C with better structure”, not as a hosted language.

---

### x86 Assembly (i386)

**Status:** Mandatory  
**Usage:** Boot, CPU control, low-level hardware

Assembly is used where C/C++ cannot safely or correctly operate:

- Boot code (Multiboot / early setup)
- GDT / IDT setup
- Context switching
- Interrupt and exception entry stubs
- Control register access (CR0/CR3/CR4)
- AP startup trampoline (SMP)

**Architecture:** 32-bit x86 (i386)

---

## Scripting & User-Level Languages

### Chrysalis Script (.csr / .chs)

**Status:** Built-in scripting  
**Usage:** Shell automation and system scripting

Chrysalis OS includes a minimal scripting system:

- `.csr` / `.chs` scripts
- Line-by-line execution
- Dispatches directly to shell commands
- No VM or bytecode layer

Designed for:
- Automation
- Boot scripts
- Simple system logic

This is **not** a general-purpose language.

---

## Build & Tooling Languages

### Make

**Usage:** Build system

The build system is written in Make and Makefiles.

- Explicit object lists
- No opaque build generators
- Easy to audit and modify

---

### Shell (sh / bash)

**Usage:** Host-side tooling

Used for:
- Build helpers
- Image generation
- Development scripts

Not part of the OS runtime.

---

## Languages Explicitly Not Used

Chrysalis OS intentionally avoids:

- Rust
- Go
- Python (runtime)
- Java
- C#
- Any language requiring a runtime or garbage collector

**Reason:**
- Full control
- Deterministic behavior
- Minimal toolchain requirements
- Easier low-level debugging

---

## Design Principles

- Every language choice is intentional
- No hidden runtimes
- No magic abstractions
- Debuggability over convenience

If a feature requires a new language, it must justify its cost.

---

## Modder Note

Forks and distributions may introduce additional languages,
but the **upstream Chrysalis OS kernel** will remain focused on:

> C + restricted C++ + x86 Assembly

---

End of file
