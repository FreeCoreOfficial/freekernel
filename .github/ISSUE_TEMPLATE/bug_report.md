---
name: Bug report
about: Report a kernel bug, crash, or unexpected behavior
title: "[BUG] "
labels: bug
assignees: ''

---

## Describe the bug

Provide a clear and concise description of the issue.
Explain **what happens**, not just that it “crashes”.

Example:
- kernel panic after enabling paging
- triple fault after enabling IRQs
- wrong behavior in scheduler or driver

---

## Reproduction steps

Describe **exactly** how to reproduce the bug.

1. Boot Chrysalis OS with configuration X
2. Enable feature Y (e.g. TASKS_ENABLED, ACPI, paging)
3. Run command / trigger interrupt / wait N ticks
4. Observe crash or incorrect behavior

If the bug is intermittent, mention that.

---

## Expected behavior

Describe what you expected to happen instead.

Example:
- system should continue booting
- task switch should occur without crash
- IRQ should be handled and return normally

---

## Actual behavior

Describe what actually happens.

- Panic message (if any)
- Reboot / freeze / triple fault
- Wrong output or corrupted state

---

## Logs / Output

Paste **serial output**, kernel logs, or panic screens here.

If available:
- COM1 serial log
- panic screen text
- register dump (EIP, ESP, CR2, etc.)

```text
PASTE LOGS HERE
````

---

## Environment

Please fill in as much as possible.

* Host OS: (e.g. Linux, distro + version)
* Emulator / Hardware: (QEMU, Bochs, real hardware)
* CPU architecture: (i386, x86_64 host, etc.)
* QEMU command line (if applicable):

```text
qemu-system-i386 ...
```

---

## Kernel configuration

* Git commit / branch:
* Enabled features:

  * [ ] Paging
  * [ ] Tasks / Scheduler
  * [ ] User mode
  * [ ] ACPI
  * [ ] SMP
* Custom modifications: yes / no
  (If yes, briefly describe)

---

## Additional context

Add any extra information that may help debugging:

* recent code changes
* suspected subsystem
* links to relevant files or functions

---

⚠️ **Important**

Kernel bugs are often timing- or state-dependent.
The more precise the report, the higher the chance it can be fixed.

