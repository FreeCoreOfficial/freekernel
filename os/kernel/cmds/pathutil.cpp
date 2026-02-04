#include "pathutil.h"
#include "cd.h"
#include "../string.h"

static void append_path(char* dst, size_t cap, const char* src) {
    size_t len = strlen(dst);
    size_t i = 0;
    while (src[i] && (len + 1) < cap) {
        dst[len++] = src[i++];
    }
    dst[len] = 0;
}

extern "C" void cmd_resolve_path(const char* input, char* out, size_t out_size) {
    if (!out || out_size == 0) return;
    out[0] = 0;

    if (!input || !*input) return;

    if (input[0] == '/') {
        strncpy(out, input, out_size);
        out[out_size - 1] = 0;
        return;
    }

    char cwd[256];
    cd_get_cwd(cwd, sizeof(cwd));
    strncpy(out, cwd, out_size);
    out[out_size - 1] = 0;

    size_t len = strlen(out);
    if (len > 0 && out[len - 1] != '/') {
        append_path(out, out_size, "/");
    }
    append_path(out, out_size, input);
}
