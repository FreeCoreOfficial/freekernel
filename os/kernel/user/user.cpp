#include "user.h"
#include "../terminal.h"
#include <stdint.h>
#include <stddef.h> /* pentru size_t */

/* mici utilitare string/memcpy pentru freestanding (kernel) */
static void *k_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char*)s;
    for (size_t i = 0; i < n; ++i) p[i] = (unsigned char)c;
    return s;
}

/* copie sigură: termină cu '\0' (trunchiază dacă e necesar) */
static char *k_strncpy(char *dst, const char *src, size_t maxlen) {
    size_t i = 0;
    if (maxlen == 0) return dst;
    while (i < maxlen - 1 && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

/* comparare similară cu strncmp (return 0 dacă egale) */
static int k_strncmp(const char *a, const char *b, size_t n) {
    size_t i = 0;
    if (n == 0) return 0;
    while (i < n && a[i] && b[i]) {
        if (a[i] != b[i]) return (int)((unsigned char)a[i]) - (int)((unsigned char)b[i]);
        i++;
    }
    if (i == n) return 0;
    return (int)((unsigned char)a[i]) - (int)((unsigned char)b[i]);
}

/* Wrapper-uri pentru confort (înlocuiesc memset/strncpy/strncmp din fișier) */
#define memset_k(dest, val, n) k_memset((dest), (val), (n))
#define strncpy_k(dest, src, n) k_strncpy((dest), (src), (n))
#define strncmp_k(a, b, n) k_strncmp((a), (b), (n))

#define MAX_USERS 8

static user_t users[MAX_USERS];
static int users_count = 0;
static user_t* current_user = 0;

static int find_user_index(const char* name) {
    for (int i = 0; i < users_count; ++i) {
        if (strncmp_k(users[i].name, name, sizeof(users[i].name)) == 0)
            return i;
    }
    return -1;
}

int user_create(const char* name, uint32_t uid, const char* password, const char* home, int is_system) {
    if (users_count >= MAX_USERS) return -1;
    user_t* u = &users[users_count++];
    u->uid = uid;
    u->gid = (is_system ? 0 : uid);
    u->is_system = is_system ? 1 : 0;
    /* safe copies */
    memset_k(u->name, 0, sizeof(u->name));
    strncpy_k(u->name, name, sizeof(u->name));
    memset_k(u->password, 0, sizeof(u->password));
    if (password) strncpy_k(u->password, password, sizeof(u->password));
    memset_k(u->home, 0, sizeof(u->home));
    if (home) strncpy_k(u->home, home, sizeof(u->home));
    return 0;
}

int user_switch(const char* name, const char* password) {
    int idx = find_user_index(name);
    if (idx < 0) return -1;
    user_t* u = &users[idx];
    /* If stored password is empty, allow switch without password */
    if (u->password[0] == '\0') {
        current_user = u;
        terminal_printf("User switched to '%s' (no password)\n", u->name);
        return 0;
    }
    if (password && strncmp_k(u->password, password, sizeof(u->password)) == 0) {
        current_user = u;
        terminal_printf("User switched to '%s'\n", u->name);
        return 0;
    }
    return -1;
}

user_t* user_get_current(void) {
    return current_user;
}

void user_init(void) {
    /* clear */
    users_count = 0;
    current_user = 0;

    /* default system user: name "system", uid=0, password "123" */
    user_create("system", 0, "123", "/", 1);

    /* set current user to system by default */
    if (users_count > 0) {
        current_user = &users[0];
        terminal_printf("[user] default user = '%s' (uid=%u)\n", current_user->name, current_user->uid);
    }
}
