/* kernel/cmds/cs.cpp
 * Chrysalis Script Interpreter (/bin/cs)
 *
 * Implements a simple line-by-line interpreter for .csr and .chs files.
 * Runs in "user mode" context (invoked via exec).
 */

#include "cs.h"
#include "../terminal.h"
#include "../string.h"
#include "../mem/kmalloc.h"
#include "registry.h"
#include "fat.h"
#include "../time/timer.h"
#include "pathutil.h"

/* FAT32 Driver API */
extern "C" int fat32_read_file(const char* path, void* buf, uint32_t max_size);

/* Import serial logging */
extern "C" void serial(const char *fmt, ...);

/* Helper to execute a single command line */
static void cs_exec_line(char* line, int line_num) {
    /* Trim leading whitespace */
    while (*line == ' ' || *line == '\t') line++;

    /* Trim trailing whitespace (newlines, CRs, spaces) */
    int len = strlen(line);
    while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r' || line[len-1] == ' ' || line[len-1] == '\t')) {
        line[len-1] = 0;
        len--;
    }

    /* Ignore empty lines and comments */
    if (*line == 0 || *line == '#') return;

    serial("[CS] Executing line %d: %s\n", line_num, line);

    /* Tokenize */
    char* argv[32];
    int argc = 0;
    char* p = line;

    while (*p && argc < 32) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == 0) break;

        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = 0;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = 0;
        }
    }

    if (argc > 0) {
        /* Dispatch command */
        /* We use the registry lookup helper */
        int found = 0;
        for (int i = 0; i < command_count; i++) {
            if (strcmp(command_table[i].name, argv[0]) == 0) {
                command_table[i].func(argc, argv);
                found = 1;
                break;
            }
        }
        if (!found) {
            terminal_printf("Script error line %d: Unknown command '%s'\n", line_num, argv[0]);
            serial("[CS] Error: Unknown command '%s'\n", argv[0]);
        }
    }
}

/* Helper to read file content (similar to exec.cpp but for text) */
static char* cs_read_script(const char* path, size_t* out_size) {
    /* Try Disk (FAT32) Only */
    fat_automount();

    size_t max_size = 64 * 1024; // 64KB script limit
    char* buf = (char*)kmalloc(max_size);
    if (buf) {
        int bytes = fat32_read_file(path, buf, max_size - 1);
        if (bytes > 0) {
            buf[bytes] = 0; // Null terminate text
            *out_size = (size_t)bytes;
            return buf;
        }
        kfree(buf);
    }
    return nullptr;
}

/* --- Simple script environment --- */

#define CS_MAX_VARS 32

typedef struct {
    char name[32];
    char value[128];
} cs_var_t;

static cs_var_t cs_vars[CS_MAX_VARS];
static int cs_var_count = 0;

static int cs_argc = 0;
static char** cs_argv = nullptr;

static void cs_set_var(const char* name, const char* value) {
    if (!name || !*name) return;
    for (int i = 0; i < cs_var_count; i++) {
        if (strcmp(cs_vars[i].name, name) == 0) {
            strncpy(cs_vars[i].value, value ? value : "", sizeof(cs_vars[i].value) - 1);
            cs_vars[i].value[sizeof(cs_vars[i].value) - 1] = 0;
            return;
        }
    }
    if (cs_var_count >= CS_MAX_VARS) return;
    strncpy(cs_vars[cs_var_count].name, name, sizeof(cs_vars[cs_var_count].name) - 1);
    cs_vars[cs_var_count].name[sizeof(cs_vars[cs_var_count].name) - 1] = 0;
    strncpy(cs_vars[cs_var_count].value, value ? value : "", sizeof(cs_vars[cs_var_count].value) - 1);
    cs_vars[cs_var_count].value[sizeof(cs_vars[cs_var_count].value) - 1] = 0;
    cs_var_count++;
}

static const char* cs_get_var(const char* name) {
    if (!name || !*name) return "";
    for (int i = 0; i < cs_var_count; i++) {
        if (strcmp(cs_vars[i].name, name) == 0) return cs_vars[i].value;
    }
    return "";
}

static void cs_expand_vars(const char* in, char* out, int out_size) {
    int oi = 0;
    for (int i = 0; in && in[i] && oi < out_size - 1; i++) {
        if (in[i] == '$') {
            i++;
            if (!in[i]) break;
            if (in[i] >= '0' && in[i] <= '9') {
                int idx = in[i] - '0';
                const char* rep = (idx < cs_argc) ? cs_argv[idx] : "";
                while (*rep && oi < out_size - 1) out[oi++] = *rep++;
            } else if (in[i] == '#') {
                char buf[8];
                itoa_dec(buf, cs_argc > 0 ? (cs_argc - 1) : 0);
                for (int k = 0; buf[k] && oi < out_size - 1; k++) out[oi++] = buf[k];
            } else {
                char name[32];
                int ni = 0;
                while (in[i] && ((in[i] >= 'a' && in[i] <= 'z') || (in[i] >= 'A' && in[i] <= 'Z') || (in[i] >= '0' && in[i] <= '9') || in[i] == '_')) {
                    if (ni < (int)sizeof(name) - 1) name[ni++] = in[i];
                    i++;
                }
                i--;
                name[ni] = 0;
                const char* rep = cs_get_var(name);
                while (*rep && oi < out_size - 1) out[oi++] = *rep++;
            }
        } else {
            out[oi++] = in[i];
        }
    }
    out[oi] = 0;
}

static int cs_eval_condition(const char* op, const char* a, const char* b) {
    if (strcmp(op, "eq") == 0) return strcmp(a, b) == 0;
    if (strcmp(op, "ne") == 0) return strcmp(a, b) != 0;
    if (strcmp(op, "exists") == 0) {
        char path[256];
        cmd_resolve_path(a, path, sizeof(path));
        fat_automount();
        return fat32_get_file_size(path) >= 0;
    }
    if (strcmp(op, "isdir") == 0) {
        char path[256];
        cmd_resolve_path(a, path, sizeof(path));
        fat_automount();
        return fat32_directory_exists(path) != 0;
    }
    return 0;
}

static int cs_tokenize(char* line, char** argv, int max_args) {
    int argc = 0;
    char* p = line;
    while (*p && argc < max_args) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (*p == '"') {
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = 0;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
            if (*p) *p++ = 0;
        }
    }
    return argc;
}

static int cs_run_script(const char* path, int argc, char** argv, int depth);

static int cs_handle_builtin(char** argv, int argc, int* out_jump) {
    *out_jump = 0;
    if (argc == 0) return 0;

    if (strcmp(argv[0], "set") == 0) {
        if (argc >= 2) {
            char* eq = strchr(argv[1], '=');
            if (eq) {
                *eq = 0;
                cs_set_var(argv[1], eq + 1);
            } else if (argc >= 3) {
                cs_set_var(argv[1], argv[2]);
            }
        }
        return 1;
    }

    if (strcmp(argv[0], "echo") == 0) {
        if (argc >= 2) {
            for (int i = 1; i < argc; i++) {
                terminal_writestring(argv[i]);
                if (i + 1 < argc) terminal_writestring(" ");
            }
        }
        terminal_writestring("\n");
        return 1;
    }

    if (strcmp(argv[0], "sleep") == 0) {
        if (argc >= 2) {
            int ms = atoi(argv[1]);
            if (ms < 0) ms = 0;
            sleep((uint32_t)ms);
        }
        return 1;
    }

    if (strcmp(argv[0], "include") == 0) {
        if (argc >= 2) {
            char resolved[256];
            cmd_resolve_path(argv[1], resolved, sizeof(resolved));
            cs_run_script(resolved, cs_argc, cs_argv, 1);
        }
        return 1;
    }

    return 0;
}

static int cs_run_script(const char* path, int argc, char** argv, int depth) {
    (void)argc;
    (void)argv;
    if (depth > 4) return -1;
    size_t script_size = 0;
    char* script_data = cs_read_script(path, &script_size);
    if (!script_data) return -1;

    char* lines[1024];
    int line_count = 0;
    char* p = script_data;
    char* line_start = p;

    while (*p && line_count < 1024) {
        if (*p == '\n') {
            *p = 0;
            lines[line_count++] = line_start;
            line_start = p + 1;
        }
        p++;
    }
    if (*line_start && line_count < 1024) {
        lines[line_count++] = line_start;
    }

    for (int i = 0; i < line_count; i++) {
        char expanded[512];
        cs_expand_vars(lines[i], expanded, sizeof(expanded));

        char tmp[512];
        strncpy(tmp, expanded, sizeof(tmp));
        tmp[sizeof(tmp) - 1] = 0;

        char* argv_local[32];
        int argc_local = cs_tokenize(tmp, argv_local, 32);
        if (argc_local == 0) continue;
        if (argv_local[0][0] == '#') continue;

        if (strcmp(argv_local[0], "if") == 0 && argc_local >= 3) {
            const char* op = argv_local[1];
            const char* a = argv_local[2];
            const char* b = (argc_local >= 4) ? argv_local[3] : "";
            if (!cs_eval_condition(op, a, b)) {
                int depth_if = 1;
                while (i + 1 < line_count && depth_if > 0) {
                    i++;
                    char tmp2[64];
                    cs_expand_vars(lines[i], tmp2, sizeof(tmp2));
                    if (strncmp(tmp2, "if ", 3) == 0) depth_if++;
                    else if (strncmp(tmp2, "endif", 5) == 0) depth_if--;
                }
            }
            continue;
        }

        if (strcmp(argv_local[0], "endif") == 0) {
            continue;
        }

        if (strcmp(argv_local[0], "for") == 0 && argc_local >= 4) {
            const char* var = argv_local[1];
            int start = atoi(argv_local[2]);
            int end = atoi(argv_local[3]);
            int loop_start = i + 1;
            int depth_for = 1;
            int loop_end = -1;
            for (int j = loop_start; j < line_count; j++) {
                char tmp2[64];
                cs_expand_vars(lines[j], tmp2, sizeof(tmp2));
                if (strncmp(tmp2, "for ", 4) == 0) depth_for++;
                else if (strncmp(tmp2, "endfor", 6) == 0) {
                    depth_for--;
                    if (depth_for == 0) { loop_end = j; break; }
                }
            }
            if (loop_end == -1) continue;
            for (int v = start; v <= end; v++) {
                char vbuf[16];
                itoa_dec(vbuf, v);
                cs_set_var(var, vbuf);
                for (int j = loop_start; j < loop_end; j++) {
                    char expanded2[512];
                    cs_expand_vars(lines[j], expanded2, sizeof(expanded2));
                    cs_exec_line(expanded2, j + 1);
                }
            }
            i = loop_end;
            continue;
        }

        if (strcmp(argv_local[0], "endfor") == 0) {
            continue;
        }

        int jump = 0;
        if (cs_handle_builtin(argv_local, argc_local, &jump)) continue;

        cs_exec_line(expanded, i + 1);
    }

    kfree(script_data);
    return 0;
}

extern "C" int cmd_cs_main(int argc, char** argv) {
    serial("[CS] Interpreter started.\n");

    if (argc < 2) {
        terminal_writestring("Usage: cs <script.csr>\n");
        return -1;
    }

    cs_argc = argc - 1;
    cs_argv = argv + 1;

    const char* script_path = argv[1];
    serial("[CS] Opening script: %s\n", script_path);

    int r = cs_run_script(script_path, argc, argv, 0);
    if (r != 0) {
        terminal_printf("cs: cannot open script '%s'\n", script_path);
        serial("[CS] Error: Cannot open script.\n");
        return -1;
    }

    serial("[CS] Script terminated.\n");
    return 0;
}
