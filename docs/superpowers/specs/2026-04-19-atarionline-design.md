# AtariOnline Browser Feature — Design Spec

**Date:** 2026-04-19  
**Project:** atari++ emulator  
**Feature:** In-emulator browser for atarionline.pl disk image library

---

## Overview

Add an "AtariOnline..." entry to the title-bar quick menu. When selected, it opens an in-emulator browser that connects to `https://atarionline.pl/v01/index.php?ct=kazip`, lets the user navigate the directory hierarchy, download a disk image (`.atr`, `.xex`, `.cas`, `.car`, `.bin`, `.xfd`, `.dcm`) to `~/.atari++/downloads/`, mounts it as Drive 1 (D1:), and cold-starts the emulator.

---

## Navigation Structure

The AtariOnline Kaz catalog has three navigation levels, all served by the same PHP script:

| Level | URL | Content |
|-------|-----|---------|
| 0 — Letters | `index.php?ct=kazip` | Links to letter groups: `0-9`, `A`…`Z` |
| 1 — Game titles | `index.php?ct=kazip&sub=B` | Links to individual game title entries |
| 2 — Files | `index.php?ct=kazip&sub=B&title=+Bandit+Boulderdash` | Downloadable files for that title |

File download URL pattern:  
`https://atarionline.pl/v01/kazip.php?ct=kazip&sub=B&title=+Bandit+Boulderdash&file=Bandit+Boulderdash.atr`

Note: "sub-titles" (e.g., "Bandit Boulderdash 10", "Bandit Boulderdash 11") are **separate top-level game title entries** in the same letter page — not sub-directories under "Bandit Boulderdash". The navigation depth is always exactly 3 levels (letter → title → file).

---

## HTML Parsing

HTTP is fetched via `popen("curl -s 'URL'", "r")`. Parsing uses plain `strstr`/`strchr` — no STL, consistent with the project's coding style.

**Level 0 — extract letter links:**
```
Search: href="index.php?ct=kazip&amp;sub=
Extract: substring up to next '"'
Example: sub=B  →  letter "B"
```

**Level 1 — extract game title links:**
```
Search: href="index.php?ct=kazip&amp;sub=LETTER&amp;title=
Extract: URL-encoded title up to '#kaz_...'
URL-decode: '+' → ' ', '%XX' → char, strip leading space
Example: +Bandit+Boulderdash  →  "Bandit Boulderdash"
Keep encoded form for URL construction.
```

**Level 2 — extract file links:**
```
Search: href="kazip.php?ct=kazip&amp;sub=LETTER&amp;title=TITLE&amp;file=
Extract: URL-encoded filename up to '"'
URL-decode for display; keep encoded form for download URL.
Supported extensions: .atr .xex .cas .car .bin .xfd .dcm
```

**Duplicate suppression:** Level 1 parsing may produce the same letter prefix repeated from the navigation bar. Only entries whose `sub=` value matches the current letter are accepted. Level 0 also deduplicates (the nav bar links appear multiple times on each page).

---

## New Class: `AtariOnlineRequester`

File: `atarionline.hpp` / `atarionline.cpp`  
Base class: `Requester` (same as `FileRequester`)

### State
```cpp
int   NavLevel;        // 0, 1, or 2
char *CurrentSub;      // URL-encoded letter, e.g. "B" or "0-9"
char *CurrentTitle;    // URL-encoded title, e.g. "+Bandit+Boulderdash"
char *DownloadDir;     // "~/.atari++/downloads" (expanded at init)
char *SelectedFile;    // full path of downloaded file (result)
```

### Item list (navigation entries)
```cpp
struct NavItem : public Node<NavItem> {
  enum Type { BACK, ENTRY, FILE } ItemType;
  char DisplayName[256];  // shown in list
  char UrlParam[512];     // raw URL value (URL-encoded)
};
List<NavItem> Items;
```

Items always start with a `BACK` entry (disabled at level 0). `ENTRY` items are letters or game titles; `FILE` items are downloadable files.

### UI gadgets
- `TextGadget` — title bar: "AtariOnline — Kaz Catalog"
- `VerticalGroup` inside a clipped `RenderPort` — scrollable list of `ButtonGadget` items, one per `NavItem`
- `ButtonGadget` — "Cancel"

Each list `ButtonGadget` carries its index in `ControlId` so `HandleEvent` can identify which item was clicked.

### Methods
```
void   PopulateLevel0(const char *html)   // parse letters
void   PopulateLevel1(const char *html)   // parse game titles
void   PopulateLevel2(const char *html)   // parse file names
bool   FetchAndPopulate(const char *url)  // curl + parse + rebuild gadgets
bool   DownloadFile(const char *sub, const char *title, const char *file)
                                           // curl -L -o ~/.atari++/downloads/file URL
void   RebuildBrowser()                   // destroy + recreate list gadgets
```

### Request flow
1. `Request()` is called by `TitleMenu`
2. `BuildGadgets()` calls `FetchAndPopulate(level0_url)` to populate letters
3. Event loop: user clicks an item
   - `BACK` → decrement level, re-fetch previous URL
   - `ENTRY` at level 0 → set `CurrentSub`, fetch level 1 URL
   - `ENTRY` at level 1 → set `CurrentTitle`, fetch level 2 URL
   - `FILE` at level 2 → call `DownloadFile()`, store path in `SelectedFile`, return success
   - Cancel → return 0

### curl commands
```sh
# Fetch HTML:
curl -s --max-time 15 'https://atarionline.pl/v01/index.php?ct=kazip&sub=B'

# Download file:
curl -L --max-time 60 -o '/home/user/.atari++/downloads/file.atr' \
  'https://atarionline.pl/v01/kazip.php?ct=kazip&sub=B&title=+Bandit+Boulderdash&file=Bandit+Boulderdash.atr'
```

Error handling: if `popen` fails or curl returns no data, show a `WarningRequester` with the error message and return 0.

---

## Changes to Existing Classes

### `DiskDrive` (diskdrive.hpp / diskdrive.cpp)

Add one public method:
```cpp
void MountImage(const char *path);
```
Implementation: eject current disk, allocate new `ImageToLoad` (consistent with existing `new char[]` pattern), then call `InsertDisk(false)`.

### `Machine` (machine.hpp / machine.cpp)

Add private array and public accessor:
```cpp
// machine.hpp (private):
class DiskDrive *DriveChain[4];

// machine.hpp (public):
class DiskDrive *DriveOf(int id);  // id 0–3, returns NULL if out of range
```

In `BuildMachine()`, assign drives to `DriveChain` before passing to SIO:
```cpp
DriveChain[0] = new class DiskDrive(this, "Drive.1", 0);
sio->RegisterDevice(DriveChain[0]);
// ... repeat for 1–3
```

### `TitleMenu` (titlemenu.hpp / titlemenu.cpp)

**titlemenu.hpp:**
- Add `TA_AtariOnline` to `enum ActionId`
- Add `class AtariOnlineRequester *AtariOnlineReq` member

**titlemenu.cpp:**
- `#include "atarionline.hpp"`
- In constructor: `AtariOnlineReq(new class AtariOnlineRequester(mach))`
- In destructor: `delete AtariOnlineReq`
- In `CollectTopics()`: add menu item after "Cold Start" separator:
  ```cpp
  new class MenuActionItem(projectmenu, "AtariOnline...", TA_AtariOnline);
  ```
- In `EnterMenu()` switch/case:
  ```cpp
  case TA_AtariOnline:
    RemoveMenu();
    if (AtariOnlineReq->Request()) {
      Machine->DriveOf(0)->MountImage(AtariOnlineReq->SelectedItem());
      Machine->ColdStart();
    }
    break;
  ```

### `makefile` / `makefile.in`

Add `atarionline.o` to the object list (same location as other requester `.o` files).

---

## Download Directory

Path: `$HOME/.atari++/downloads/`  
Created at first use via `mkdir -p` equivalent (using POSIX `mkdir` + `stat` if not exist).  
Files are kept after session — user can reuse previously downloaded images.  
No cleanup is performed by the emulator.

---

## Error Cases

| Situation | Behaviour |
|-----------|-----------|
| No network / curl fails | `WarningRequester` "Could not connect to atarionline.pl" |
| Empty page (0 items parsed) | `WarningRequester` "No entries found" |
| Download fails (curl exit ≠ 0) | `WarningRequester` "Download failed" |
| File extension not supported | Item not shown (filtered during parse) |
| Drive 1 busy / image load error | Existing `DiskDrive` exception handling propagates |

---

## Supported File Extensions

`.atr` `.xex` `.cas` `.car` `.bin` `.xfd` `.dcm`

---

## Out of Scope

- Search / filtering within a letter page
- Caching of fetched HTML between sessions
- Drive selection (always D1:)
- Progress bar during download (curl runs synchronously inside `popen`)
- HTTPS certificate verification (curl default behaviour applies)
