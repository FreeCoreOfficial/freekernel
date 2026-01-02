# disk-usage.md

Comanda `disk` — documentație completă pentru Chrysalis OS
(versiune pentru `kernel/cmds/disk.cpp` — funcționalități: list, init, assign, format, test)

---

## Overview (pe scurt)

`disk` este un utilitar shell minimalist pentru diagnostic și operații simple pe dispozitivul ATA atașat.
Funcționalități principale:

* afișează informații IDENTIFY ale dispozitivului
* creează un MBR minimal (opțiune `--init`)
* mapare temporară `letter -> (LBA, count)` (opțiune `--assign`)
* quick-format (zero) pentru o partiție mapată (opțiune `--format`)
* teste de citire/scriere pe LBA-uri (`--test`)

Comenzile sunt scrise cu scop de testare / QEMU; folosește-le cu precauție pe hardware real.

---

## Sintaxă generală

Comanda este apelată din shell ca o singură linie cu argumente separate prin spațiu:

```
disk <subcommand> [subargs...]
```

Toate valorile LBA / count trebuie să fie **numere zecimale pozitive** (parserul intern acceptă doar cifre `0-9`).

---

## Subcomenzi — descriere, sintaxă și exemple

### `--help` / `--usage`

Afișează help-ul comenzii.

```
disk --help
disk --usage
```

**Output (exemplu)**

```
disk --list              : show device info
disk --usage / --help    : this help
disk --init              : write simple MBR (one partition, LBA start 2048)
disk --assign <letter> <lba> <count> : assign letter 'a' to an LBA range
disk --format <letter>   : quick-zero first sectors of the partition
disk --test --write <lba>: write test pattern to lba
disk --test --output <lba>: read & dump LBA
disk --test --see-raw <lba> <count> : dump raw sectors
```

---

### `--list`

Afișează modelul / capacitatea (LBA28) obținută prin `ata_identify`, și lista asignărilor curente (în memorie).

```
disk --list
```

**Exemplu output**

```
[disk] model: QEMU HARDDISK
[disk] LBA28 sectors: 2097152
[disk] assignments:
  a => LBA 2048 count 100000
```

**Note**

* Dacă `ata_identify` eșuează, afișează `[disk] no device / identify failed`.
* `assignments` sunt stocate doar în RAM (variabila `g_assigns`) — nu persistă după reboot.

---

### `--init`

Scrie un MBR minimal pe sectorul 0 (MBR). Creează o singură partiție de tip `0x0B` (FAT32 CHS/LBA), început la LBA `2048` până la finalul discului. Folosește `ata_set_allow_mbr_write(1)` pentru a permite scrierea temporară.

```
disk --init
```

**Exemplu output**

```
[disk] initializing (write MBR) ...
[disk] MBR written (partition created)
```

**Return / erori posibile**

* `[disk] identify failed - cannot init MBR` — `ata_identify` a eșuat
* `[disk] identify returned 0 sectors` — dispozitivul raportează 0 sectoare
* `[disk] MBR write FAILED` — `ata_write_sector(0, ...)` a eșuat

**Safety**

* Scrierea MBR rescrie tabelul partițiilor; folosește doar pe dispozitive de test (QEMU) sau după backup complet.

---

### `--assign <letter> <lba> <count>`

Mapează temporar o literă (de ex. `a`) la un offset LBA și un număr de sectoare. Maparea este **volatile** (RAM only).

```
disk --assign a 2048 100000
```

**Output**

```
[disk] assigned
```

**Validări**

* `letter` trebuie să fie între `'a'` și `'z'` (lowercase). Alte valori sunt considerate invalide.
* `lba` și `count` parsează doar cifre (fără semne sau hex).

**Utilitate**

* Folosit de `--format <letter>` pentru a ști ce interval să zeroze.

---

### `--format <letter>`

Quick-format: scrie zerouri în primele N sectoare ale partiției mapate. `N` = `min(count, 128)`.

```
disk --format a
```

**Output**

* Mesaje privind progresul, eșecuri sau succes:

```
[disk] quick-format done (32 sectors zeroed)
```

**Erori**

* `[disk] invalid letter` — literă invalidă
* `[disk] letter not assigned` — litera nu e mapată
* `[disk] write failed at LBA X (r=Y)` — scrierea sectorului X a eșuat (r=cod return)

**Note**

* Această funcție scrie `128` sectoare max (de ex pentru a nu tăia o partiție mare din greșeală).
* Este rapidă dar **distructivă** — face zero primele blocuri.

---

### `--test --write <lba>`

Scrie un pattern de test (0x00..0xFF repetat) pe sectorul LBA specificat.

```
disk --test --write 12345
```

**Output**

```
[disk] test write OK
```

sau

```
[disk] test write FAILED
```

**Note**

* Utilizează `ata_write_sector(lba, buf)`; verifică driverul ATA dacă returnează erori.

---

### `--test --output <lba>`

Citește un singur sector și afișează un hexdump ( valoare implicită: 512 bytes → hexdump de 256 octeți pe 16 coloane în implementarea curentă ).

```
disk --test --output 12345
```

**Output (exemplu fragment)**

```
--- LBA 12345 ---
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 
10 11 12 13 ... (și așa mai departe)
```

**Eroare**

* `[disk] read failed` — `ata_read_sector` a eșuat

---

### `--test --see-raw <lba> <count>`

Dump raw pentru `count` sectoare începând de la `lba`. Afișează titlu `--- LBA N ---` pentru fiecare sector, urmat de hexdump.

```
disk --test --see-raw 2048 4
```

**Output (exemplu)**

```
--- LBA 2048 ---
...hexdump sector 2048...
--- LBA 2049 ---
...hexdump sector 2049...
...
```

**Eroare**

* Dacă o citire eșuează pentru un sector, afișează `[disk] read failed` și oprește bucla.

---

## Mesaje/Exit codes și ce înseamnă

Comanda `cmd_disk` folosește `println` / `print` pentru a raporta starea. Majoritatea subcomenzilor returnează `void` la nivel de shell, dar intern pot întoarce coduri de eroare (ex.: `create_minimal_mbr()` returnează < 0 la eroare). În practică, tu interpretezi ieșirea text ca stare.

Exemple de mesaje importante:

* `[disk] no device / identify failed` ← problema driver ATA / cablaj QEMU
* `[disk] MBR written (partition created)` ← succes init
* `[disk] quick-format done (...)` ← succes format
* `[disk] write failed at LBA X (r=Y)` ← eroare la scriere (cod dev/driver în `r`)
* `[disk] read failed` ← eroare la citire

---

## Detalii de implementare (pentru dezvoltatori)

* `disk` folosește aceste apeluri ATA (trebuie definite în `storage/ata.h` / `storage/ata.c`):

  * `int ata_identify(uint16_t out[256]);` — return 0 la succes
  * `void ata_decode_identify(const uint16_t idbuf[256], char* model, size_t model_size, uint32_t* out_sectors);`
  * `int ata_read_sector(uint32_t lba, uint8_t* buf);` — return 0 la succes
  * `int ata_write_sector(uint32_t lba, const uint8_t* buf);` — return 0 la succes
  * `void ata_set_allow_mbr_write(int allow);` — permite temporar scriere pe sectorul 0
* Arg parser intern `split_args(...)` copiază șirul într-un buffer stack și separă tokenii după spațiu (max 16 tokeni).
* Parserul numeric intern `parse_u32(const char*)` acceptă doar cifre '0'-'9' (fără `+`/`-`/`0x`).
* `g_assigns[26]` — tabel in-memory ce stochează `used, letter, lba, count`. Nu este persistat.

---

## Safety / Warnings (citeste înainte să rulezi)

* `disk --init` scrie MBR — **dacă rulezi pe un disc real va distruge tabela partițiilor**. BACKUP înainte.
* `disk --format` scrie zerouri — distruge date. Nu folosi pe partitii cu date importante.
* Testele de scriere pot corupe date. Folosește imagini QEMU sau dispozitive de test.
* `ata_set_allow_mbr_write(1)` este folosit doar punctual; asigură-te că nu rămâne activ.

---

## Troubleshooting rapid

1. **`[disk] no device / identify failed`**

   * Verifică driverul ATA (`ata.c`), semnăturile funcțiilor, conexiunea hardware (QEMU a fost lansat cu un HDD?)
   * Rulează `cmd_list();` și verifică return code din `ata_identify`.

2. **`MBR write FAILED`**

   * Verifică dacă `ata_write_sector(0, mbr)` returnează coduri. Asigură-te că `ata_set_allow_mbr_write(1)` a fost apelat (și funcționează).

3. **Linker/compilare**

   * Dacă folosești build freestanding, s-ar putea să ai nevoie de implementări interne pentru `memcpy`, `sprintf` (am inclus variante minime în `disk.cpp` reparat).
   * Asigură-te că fișierul este compilat pentru i386 (Makefile trebuie să folosească `-m32` sau cross-compiler).

4. **He xdumps arată ciudat**

   * He xdumpul formatat pe 16 byte/linie. Dacă ai nevoie altfel, modifică `hexdump_line`.

---

## Exemple complete (session snippets)

1. **List device + show no assignments**

```
> disk --list
[disk] model: QEMU HARDDISK
[disk] LBA28 sectors: 2097152
[disk] assignments:
```

2. **Assign partition 'a' and format**

```
> disk --assign a 2048 100000
[disk] assigned

> disk --list
[disk] model: QEMU HARDDISK
[disk] LBA28 sectors: 2097152
[disk] assignments:
  a => LBA 2048 count 100000

> disk --format a
[disk] quick-format done (128 sectors zeroed)
```

3. **Write test pattern to sector and read it back**

```
> disk --test --write 2050
[disk] test write OK

> disk --test --output 2050
--- LBA 2050 ---
00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
10 11 12 13 ...
```

4. **Create minimal MBR (QEMU only unless you know what you do)**

```
> disk --init
[disk] initializing (write MBR) ...
[disk] MBR written (partition created)
```

---

## Sugestii de îmbunătățiri (opțional)

* Persistența `g_assigns` în MBR sau într-un fișier special.
* Comandă `--unassign <letter>` / `--show-assign <letter>`
* Extindere `k_sprintf` pentru formatare (hex, padding).
* Validări suplimentare: verificare `lba + count <= disk_sectors`.
* Opțiune `--format --full` care iterează toată partiția (atentie: lent și distrugător).

---

