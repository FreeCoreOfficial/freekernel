## 🦋 FlyUI — Chrysalis OS GUI Toolkit

### Overview

FlyUI is a minimal, kernel-side GUI toolkit designed for Chrysalis OS.
It provides widgets, event dispatch, and drawing logic while remaining independent from hardware and window management.

---

## Responsibilities

### FlyUI DOES:

* Manage widget trees
* Dispatch input events
* Draw widgets into window surfaces
* Handle focus and redraw logic
* Provide basic widgets

### FlyUI DOES NOT:

* Create or manage windows
* Access GPU or framebuffer directly
* Handle hardware input
* Perform scheduling

---

## Architecture

```
INPUT → WM → FlyUI → Widgets → Surface → Compositor → Framebuffer
```

FlyUI sits strictly **between WM and Compositor**.

---

## Widget System

All UI elements are widgets.

```c
typedef struct FlyWidget {
    int x, y, w, h;
    bool visible;
    bool needs_redraw;

    struct FlyWidget* parent;
    struct FlyWidget* children;
    struct FlyWidget* next;

    void (*draw)(struct FlyWidget*);
    void (*on_event)(struct FlyWidget*, FlyEvent*);
} FlyWidget;
```

---

## Event System

Supported events:

* Mouse move
* Mouse button down/up
* Key down/up

Events are dispatched:

1. Hit-test children
2. Deliver to top-most widget
3. Bubble to parent if unhandled

---

## Rendering Model

* Rendering is **explicit**, not automatic
* A render pass walks the widget tree
* Only widgets marked `needs_redraw` are redrawn
* Drawing is clipped to widget bounds

---

## Drawing

FlyUI drawing primitives operate on a `Surface*` provided by the Window Manager.

Supported primitives:

* Rect fill
* Border draw
* Text draw (via existing font system)

---

## Widgets (MVP)

### Label

* Displays text
* No interaction

### Button

* Rectangle with text
* Handles mouse click
* Emits serial log on click

### Panel

* Container widget
* Used for layout grouping

---

## Memory & Stability Rules

* No allocation in IRQ context
* Widgets allocated at init time
* All failures logged via serial()
* No blocking operations

---

## Extensibility

FlyUI is designed to be:

* Modder-friendly
* Easy to extend with new widgets
* Portable to userland in the future

---

## Future Direction

* Move FlyUI to userland (`/user/lib/flyui`)
* Add IPC-based event delivery
* Support themes and skins
* Add advanced widgets

