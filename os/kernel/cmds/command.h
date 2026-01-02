#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* tipul funcției comenzii: returnează int (opțional), primește argc, argv */
typedef int (*command_fn)(int argc, char **argv);

typedef struct {
    const char* name;
    command_fn func;
} Command;

#ifdef __cplusplus
}
#endif
