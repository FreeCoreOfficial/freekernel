// kernel/cmds/fat.cpp
#include "fat.h"

#include "../fs/fat/fat.h"
#include "../terminal.h"
#include "../cmds/disk.h"   // pentru g_assigns, struct part_assign și funcții din disk.cpp
#include <stdint.h>

/* Dacă disk.h nu definește MAX_ASSIGN, fallback */
#ifndef MAX_ASSIGN
#define MAX_ASSIGN 26
#endif

/* Declarații externe pentru funcțiile din disk.cpp */
extern int create_minimal_mbr(void);
extern void cmd_scan(void);
extern int cmd_format_letter(char letter);

/* Prototypes din fs/fat/fat.c */
int fat_probe(uint32_t lba);

/* strcmp local freestanding */
static int strcmp_local(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

/* Conversie uint32_t → string */
static char* u32_to_dec_local(char *out, uint32_t v) {
    char tmp[12]; int ti = 0;
    if (v == 0) { out[0] = '0'; out[1] = '\0'; return out; }
    while (v > 0) { tmp[ti++] = (char)('0' + (v % 10)); v /= 10; }
    int j = 0;
    for (int i = ti - 1; i >= 0; --i) out[j++] = tmp[i];
    out[j] = '\0';
    return out;
}

/* Help actualizat */
static void print_help(void) {
    terminal_writestring("fat - gestiune FAT filesystem\n");
    terminal_writestring("Comenzi disponibile:\n");
    terminal_writestring("  fat help                 : afiseaza acest ajutor\n");
    terminal_writestring("  fat list                 : status montare + volume FAT detectate\n");
    terminal_writestring("  fat mount <lba>          : monteaza FAT la LBA specificat\n");
    terminal_writestring("  fat info                 : informatii despre FAT-ul montat\n");
    terminal_writestring("  fat --auto-init          : AUTO: MBR + partitie + formatare + montare\n");
}

/* Afișează toate volumele FAT găsite */
static void list_all_fat_volumes(void) {
    int found = 0;

    terminal_writestring("Volume FAT detectate:\n");

    for (int i = 0; i < MAX_ASSIGN; i++) {
        if (!g_assigns[i].used) continue;

        uint32_t lba = g_assigns[i].lba;
        char letter = g_assigns[i].letter;

        if (fat_probe(lba)) {
            found = 1;
            char num[32];
            u32_to_dec_local(num, lba);

            char letter_str[4] = { letter, ':', ' ', '\0' };  // "a: "

            if (fat_is_mounted() && fat_get_mounted_lba() == lba) {
                terminal_writestring(" * ");
            } else {
                terminal_writestring("   ");
            }

            terminal_writestring(letter_str);
            terminal_writestring("LBA ");
            terminal_writestring(num);
            terminal_writestring("  (");
            if (fat_is_mounted() && fat_get_mounted_lba() == lba) {
                terminal_writestring("montat");
            } else {
                terminal_writestring("nemontat");
            }
            terminal_writestring(")\n");
        }
    }

    if (!found) {
        terminal_writestring("  Niciun volum FAT detectat.\n");
    }
}

/* Comandă principală */
int cmd_fat(int argc, char **argv)
{
    if (argc < 2) {
        print_help();
        return 0;
    }

    const char* cmd = argv[1];

    /* help */
    if (strcmp_local(cmd, "help") == 0 || strcmp_local(cmd, "--help") == 0 || strcmp_local(cmd, "-h") == 0) {
        print_help();
        return 0;
    }

    /* list */
    if (strcmp_local(cmd, "list") == 0 || strcmp_local(cmd, "--list") == 0) {
        terminal_writestring("Status FAT:\n");

        if (fat_is_mounted()) {
            char num[32];
            u32_to_dec_local(num, fat_get_mounted_lba());
            terminal_writestring("  Montat curent: LBA ");
            terminal_writestring(num);
            terminal_writestring("\n\n");
        } else {
            terminal_writestring("  Niciun FAT montat.\n\n");
        }

        list_all_fat_volumes();
        return 0;
    }

    /* fat --auto-init */
    if (strcmp_local(cmd, "--auto-init") == 0) {
        terminal_writestring("[fat] Incepe initializare automata a discului ca FAT32...\n");

        terminal_writestring("[fat] Pas 1: Creare MBR cu o singura partitie...\n");
        if (create_minimal_mbr() != 0) {
            terminal_writestring("[fat] Eroare: esec la scrierea MBR!\n");
            return 0;
        }

        terminal_writestring("[fat] Pas 2: Scanare partiții...\n");
        cmd_scan();

        if (!g_assigns[0].used) {
            terminal_writestring("[fat] Eroare: nici o partitie detectata dupa scan!\n");
            return 0;
        }

        char letter = g_assigns[0].letter;
        uint32_t part_lba = g_assigns[0].lba;

        char tmp[32];
        u32_to_dec_local(tmp, part_lba);

        terminal_writestring("[fat] Partitie detectata: ");
        char letter_str[4] = { letter, ':', ' ', '\0' };
        terminal_writestring(letter_str);
        terminal_writestring("LBA ");
        terminal_writestring(tmp);
        terminal_writestring("\n");

        terminal_writestring("[fat] Pas 3: Formatare rapida (zero primii 128 sectoare)...\n");
        if (cmd_format_letter(letter) != 0) {
            terminal_writestring("[fat] Eroare la formatare rapida!\n");
            return 0;
        }

        terminal_writestring("[fat] Pas 4: Incercare montare automata FAT...\n");
        if (fat_mount(part_lba)) {
            terminal_writestring("[fat] SUCCES TOTAL! Disc initializat si FAT montat la LBA ");
            terminal_writestring(tmp);
            terminal_writestring("\n");
            terminal_writestring("[fat] Unitatea este gata: ");
            terminal_writestring(letter_str);
            terminal_writestring("\n");
        } else {
            terminal_writestring("[fat] Montare esuata dupa formatare. Verifică hardware-ul sau rulează manual.\n");
        }

        return 0;
    }

    /* mount <lba> */
    if (strcmp_local(cmd, "mount") == 0) {
        if (argc < 3) {
            terminal_writestring("Eroare: fat mount <lba>\n");
            return 0;
        }

        uint32_t lba = 0;
        const char* p = argv[2];
        while (*p) {
            char c = *p++;
            if (c < '0' || c > '9') {
                terminal_writestring("Eroare: LBA invalid (doar cifre)\n");
                return 0;
            }
            lba = lba * 10 + (c - '0');
        }

        if (fat_mount(lba)) {
            char num[32];
            u32_to_dec_local(num, lba);
            terminal_writestring("FAT montat cu succes la LBA ");
            terminal_writestring(num);
            terminal_writestring("\n");
        } else {
            terminal_writestring("Eroare: montare FAT esuata\n");
        }
        return 0;
    }

    /* info */
    if (strcmp_local(cmd, "info") == 0) {
        if (!fat_is_mounted()) {
            terminal_writestring("Eroare: niciun FAT montat. Foloseste 'fat mount <lba>' sau 'fat --auto-init'.\n");
            return 0;
        }
        fat_info();
        return 0;
    }

    terminal_writestring("Comanda necunoscuta: ");
    terminal_writestring(cmd);
    terminal_writestring("\n");
    print_help();
    return 0;
}