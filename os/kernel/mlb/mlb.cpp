#include "mlb.h"
#include "../terminal.h"
#include "../drivers/speaker.h"

// NU include string.h! Folosim această funcție proprie:
static int mlb_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int playing = 0;
static const char* current_title = nullptr;

void mlb_print_catalog()
{
    terminal_writestring("Music Library - Powered by Mihai209:\n");
    terminal_writestring("  > mario\n");
    terminal_writestring("  > nokia\n");
    terminal_writestring("  > imperial\n");
    terminal_writestring("  > beep\n");
}

void mlb_play(const char* id)
{
    if (!id || !id[0]) {
        terminal_writestring("Usage: play <song-name>\n");
        return;
    }

    if (playing) {
        speaker_stop(); 
    }

    const char* selected_name = nullptr;

    // Folosim mlb_strcmp definit mai sus
    if (mlb_strcmp(id, "mario") == 0) {
        selected_name = "Super Mario by Mihai209";
        speaker_on(659); 
    } 
    else if (mlb_strcmp(id, "nokia") == 0) {
        selected_name = "Nokia Tune by Mihai209";
        speaker_on(660);
    }
    else if (mlb_strcmp(id, "imperial") == 0) {
        selected_name = "Imperial March by Mihai209";
        speaker_on(440);
    }
    else if (mlb_strcmp(id, "beep") == 0) {
        selected_name = "Classic Beep by Mihai209";
        speaker_on(880);
    }
    else {
        terminal_writestring("Error: Song not found in Mihai209's collection.\n");
        return;
    }

    playing = 1;
    current_title = selected_name;
    terminal_writestring("Now Playing: ");
    terminal_writestring(current_title);
    terminal_writestring("\n");
}

void mlb_stop()
{
    if (!playing) {
        terminal_writestring("Silence...\n");
        return;
    }

    speaker_stop();
    playing = 0;
    terminal_writestring("Playback stopped.\n");
}