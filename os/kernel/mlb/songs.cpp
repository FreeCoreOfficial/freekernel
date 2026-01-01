#include <stdint.h>

struct note_t {
    uint16_t freq;
    uint16_t duration_ms;
};

/* Frecvente Note Muzicale */
#define NOTE_C4 261
#define NOTE_D4 294
#define NOTE_E4 329
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 493
#define NOTE_C5 523

/* ===== SONGS ===== */

static const note_t song_mario[] = {
    {659, 150}, {659, 150}, {0, 150}, {659, 150}, {0, 150}, {523, 150}, {659, 150}, {0, 150}, {783, 150}
};

static const note_t song_nokia[] = {
    {NOTE_E5, 125}, {NOTE_D5, 125}, {NOTE_F4, 250}, {NOTE_G4, 250},
    {NOTE_C5, 125}, {NOTE_B4, 125}, {NOTE_D4, 250}, {NOTE_E4, 250}
};

static const note_t song_imperial[] = {
    {440, 500}, {440, 500}, {440, 500}, {349, 350}, {523, 150}, {440, 500}, {349, 350}, {523, 150}, {440, 650}
};

static const note_t song_beep[] = {
    {440, 300}, {0, 100}, {660, 300}, {0, 100}, {880, 400}
};

struct song_entry {
    const char* name;
    const note_t* notes;
    int count;
};

const song_entry mlb_songs[] = {
    { "Super Mario by Mihai209", song_mario, 9 },
    { "Nokia Tune by Mihai209",  song_nokia, 8 },
    { "Imperial March by Mihai209", song_imperial, 9 },
    { "Classic Beep by Mihai209", song_beep, 5 }
};

const int mlb_song_count = sizeof(mlb_songs) / sizeof(mlb_songs[0]);