#include "color.h"
#include "../terminal.h"
#include "../string.h"
#include "../colors/cl.h"
#include "../video/fb_console.h"

/* Helper to parse color name */
static int parse_color_name(const char* name) {
    if (strcmp(name, "black") == 0) return CL_BLACK;
    if (strcmp(name, "blue") == 0) return CL_BLUE;
    if (strcmp(name, "green") == 0) return CL_GREEN;
    if (strcmp(name, "cyan") == 0) return CL_CYAN;
    if (strcmp(name, "red") == 0) return CL_RED;
    if (strcmp(name, "magenta") == 0) return CL_MAGENTA;
    if (strcmp(name, "brown") == 0) return CL_BROWN;
    if (strcmp(name, "light_gray") == 0) return CL_LIGHT_GRAY;
    if (strcmp(name, "dark_gray") == 0) return CL_DARK_GRAY;
    if (strcmp(name, "light_blue") == 0) return CL_LIGHT_BLUE;
    if (strcmp(name, "light_green") == 0) return CL_LIGHT_GREEN;
    if (strcmp(name, "light_cyan") == 0) return CL_LIGHT_CYAN;
    if (strcmp(name, "light_red") == 0) return CL_LIGHT_RED;
    if (strcmp(name, "light_magenta") == 0) return CL_LIGHT_MAGENTA;
    if (strcmp(name, "yellow") == 0) return CL_YELLOW;
    if (strcmp(name, "white") == 0) return CL_WHITE;
    return -1;
}

static void print_help() {
    terminal_writestring("Usage: color <command> [args]\n");
    terminal_writestring("Commands:\n");
    terminal_writestring("  list             List available colors\n");
    terminal_writestring("  switch <fg> [bg] Set foreground (and optional background) color\n");
    terminal_writestring("  reset            Reset to default colors\n");
    terminal_writestring("  help             Show this help\n");
}

static void print_color_sample(const char* name, int fg) {
    /* Save current */
    /* Note: We don't have a get_attr yet, so we just set it back to default at end of list */
    
    fb_cons_set_attr(cl_make(fg, CL_BLACK));
    terminal_writestring(name);
    terminal_writestring(" ");
}

extern "C" int cmd_color(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 0;
    }

    const char* sub = argv[1];

    if (strcmp(sub, "help") == 0 || strcmp(sub, "--help") == 0) {
        print_help();
        return 0;
    }

    if (strcmp(sub, "reset") == 0) {
        fb_cons_set_attr(cl_default());
        terminal_writestring("Colors reset to default.\n");
        return 0;
    }

    if (strcmp(sub, "list") == 0) {
        terminal_writestring("Available colors:\n");
        print_color_sample("black", CL_BLACK);
        print_color_sample("blue", CL_BLUE);
        print_color_sample("green", CL_GREEN);
        print_color_sample("cyan", CL_CYAN);
        print_color_sample("red", CL_RED);
        print_color_sample("magenta", CL_MAGENTA);
        print_color_sample("brown", CL_BROWN);
        print_color_sample("light_gray", CL_LIGHT_GRAY);
        terminal_writestring("\n");
        print_color_sample("dark_gray", CL_DARK_GRAY);
        print_color_sample("light_blue", CL_LIGHT_BLUE);
        print_color_sample("light_green", CL_LIGHT_GREEN);
        print_color_sample("light_cyan", CL_LIGHT_CYAN);
        print_color_sample("light_red", CL_LIGHT_RED);
        print_color_sample("light_magenta", CL_LIGHT_MAGENTA);
        print_color_sample("yellow", CL_YELLOW);
        print_color_sample("white", CL_WHITE);
        terminal_writestring("\n");
        
        /* Restore default for the prompt */
        fb_cons_set_attr(cl_default());
        return 0;
    }

    if (strcmp(sub, "switch") == 0 || strcmp(sub, "set") == 0) {
        if (argc < 3) {
            terminal_writestring("Usage: color switch <fg> [bg]\n");
            return -1;
        }

        int fg = parse_color_name(argv[2]);
        if (fg < 0) {
            terminal_printf("Invalid foreground color: %s\n", argv[2]);
            return -1;
        }

        int bg = CL_BLACK; /* Default bg if not specified */
        
        /* If current default has a different BG, maybe we should preserve it? 
           For now, default to black if not specified is safer/standard. */
        if (argc >= 4) {
            bg = parse_color_name(argv[3]);
            if (bg < 0) {
                terminal_printf("Invalid background color: %s\n", argv[3]);
                return -1;
            }
        } else {
            /* Try to keep current background from default if possible, 
               but we don't have easy access to 'current' attr here without a getter.
               Let's use the background from cl_default() as a fallback if user didn't specify. */
            bg = cl_bg(cl_default());
        }

        fb_cons_set_attr(cl_make(fg, bg));
        terminal_writestring("Color changed.\n");
        return 0;
    }

    terminal_writestring("Unknown subcommand.\n");
    print_help();
    return -1;
}