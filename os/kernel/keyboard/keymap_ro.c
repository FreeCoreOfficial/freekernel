#include "keymap.h"
#include "utf.h"   // conține U_A_CIRC, U_A_BREVE, etc

/* keymap structure non-const so we can fill altgr at runtime */
static keymap_t keymap_ro = {
    .name = "ro",

    .normal = {
        0, 27,
        '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
        '\t',
        'q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',
        0,
        '\\','z','x','c','v','b','n','m',',','.','/',
        0,'*',0,' '
    },

    .shift = {
        0, 27,
        '!','@','#','$','%','^','&','*','(',')','_','+','\b',
        '\t',
        'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
        0,
        'A','S','D','F','G','H','J','K','L',':','"','~',
        0,
        '|','Z','X','C','V','B','N','M','<','>','?',
        0,'*',0,' '
    },

    /* altgr: initialize to zeros (fill special entries in init function) */
    .altgr = { 0 }
};

const keymap_t* keymap_ro_ptr = &keymap_ro;

/* Call this once at boot to populate the AltGr entries that C++ can't express statically */
void keymap_ro_init(void)
{
    /* indices are scancodes (make sure they match your scancode mapping) */
    keymap_ro.altgr[0x10] = U_A_CIRC;   /* Q -> â */
    keymap_ro.altgr[0x1E] = U_A_BREVE;  /* A -> ă */
    keymap_ro.altgr[0x17] = U_I_CIRC;   /* I -> î */
    keymap_ro.altgr[0x1F] = U_S_COMMA;  /* S -> ș */
    keymap_ro.altgr[0x14] = U_T_COMMA;  /* T -> ț */
}
