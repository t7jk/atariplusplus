# AtariOnline Browser Feature — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add "AtariOnline..." to the emulator's title-bar menu, opening an in-emulator browser for https://atarionline.pl/v01/index.php?ct=kazip that lets the user navigate the Kaz catalog, download a disk image to `~/.atari++/downloads/`, mount it as Drive 1, and cold-start the emulator.

**Architecture:** New `AtariOnlineRequester` class (parallel to existing `FileRequester`) manages HTML fetching via `popen(curl)`, parses links with `strstr`, and shows a scrollable `RequesterEntry` list using the same gadget stack as `FileList`. `TitleMenu` wires it up with a new `TA_AtariOnline` action; `DiskDrive` gets a `MountImage()` method; `Machine` exposes `DriveOf(int)` to access drive 1 by index.

**Tech Stack:** C++ (no STL), existing gadget framework (`Requester`, `RequesterEntry`, `VerticalGroup`, `RenderPort`), `popen`/`system` with `curl` CLI, POSIX `mkdir`/`stat`.

---

## File Map

| Action | File | What changes |
|--------|------|-------------|
| Create | `atarionline.hpp` | `AtariOnlineRequester` class declaration |
| Create | `atarionline.cpp` | Full implementation |
| Modify | `diskdrive.hpp` | Add `MountImage(const char *)` public declaration |
| Modify | `diskdrive.cpp` | Add `MountImage` implementation |
| Modify | `machine.hpp` | Add `DiskDrive *DriveChain[4]` (private) + `DriveOf(int)` (public) |
| Modify | `machine.cpp` | Assign DriveChain in BuildMachine; implement DriveOf |
| Modify | `titlemenu.hpp` | Add `TA_AtariOnline` enum value + `AtariOnlineReq` member |
| Modify | `titlemenu.cpp` | Include, ctor/dtor, menu item, case handler |
| Modify | `Makefile.atari` | Add `atarionline` to FILES list |

All files are in `/home/t7jk/atari/`. Build is done from `/home/t7jk/atari/atari++/` with `make -f makefile atari`.

---

## Task 1: Add `MountImage()` to DiskDrive

**Files:**
- Modify: `diskdrive.hpp` (public section, after `InsertDisk` declaration ~line 220)
- Modify: `diskdrive.cpp` (add after `InsertDisk` implementation ~line 1160)

- [ ] **Step 1: Add declaration to diskdrive.hpp**

Find the public section containing `InsertDisk` (~line 218):
```cpp
  // Load a disk, possibly throw on error, and write-protect the disk should we
  // have one.
  void InsertDisk(bool protect);
```
Add after it:
```cpp
  // Eject any current disk, set a new image path, and insert it unprotected.
  void MountImage(const char *path);
```

- [ ] **Step 2: Add implementation to diskdrive.cpp**

Find the end of `DiskDrive::InsertDisk` (~line 1158). Add after the closing `///`:

```cpp
/// DiskDrive::MountImage
// Eject current disk, set new image path, and insert it unprotected.
void DiskDrive::MountImage(const char *path)
{
  EjectDisk();
  delete[] ImageToLoad;
  ImageToLoad = NULL;
  if (path && *path) {
    ImageToLoad = new char[strlen(path) + 1];
    strcpy(ImageToLoad,path);
    InsertDisk(false);
  }
}
///
```

- [ ] **Step 3: Build to verify compilation**

```bash
cd /home/t7jk/atari/atari++ && make -f makefile diskdrive.o 2>&1 | grep -E "error:|warning:" | head -20
```
Expected: no errors (warnings about unused variables are acceptable).

- [ ] **Step 4: Commit**

```bash
cd /home/t7jk/atari && git add diskdrive.hpp diskdrive.cpp
git commit -m "feat: add DiskDrive::MountImage() for direct image mounting"
```

---

## Task 2: Expose `DriveOf()` in Machine

**Files:**
- Modify: `machine.hpp` (private section ~line 88, public section ~line 395)
- Modify: `machine.cpp` (BuildMachine ~line 265, add DriveOf implementation)

- [ ] **Step 1: Add DriveChain to machine.hpp private section**

Find in the private section (~line 93), after `List<Chip> chipChain;`:
```cpp
  List<Chip>            chipChain;
```
Add after it:
```cpp
  // Direct references to the four disk drives for fast access.
  class DiskDrive      *DriveChain[4];
```

- [ ] **Step 2: Add DriveOf() to machine.hpp public section**

Find in the public section after the `SIO()` accessor (~line 389):
```cpp
  class SIO *SIO(void) const
```
Add before it (or after the `Sound()` accessor, whichever is cleaner — just keep it in the public accessors block):
```cpp
  // Return drive by zero-based index (0 = D1:). Returns NULL if out of range.
  class DiskDrive *DriveOf(int id) const
  {
    if (id < 0 || id > 3) return NULL;
    return DriveChain[id];
  }
```

- [ ] **Step 3: Add forward declaration for DiskDrive in machine.hpp**

Near the top of machine.hpp find the forward declarations block (look for `class Printer;` etc.). Add:
```cpp
class DiskDrive;
```
if not already present.

- [ ] **Step 4: Initialize DriveChain in machine.cpp BuildMachine**

Find (~line 265):
```cpp
  sio->RegisterDevice(new class DiskDrive(this,"Drive.1",0));
  sio->RegisterDevice(new class DiskDrive(this,"Drive.2",1));
  sio->RegisterDevice(new class DiskDrive(this,"Drive.3",2));
  sio->RegisterDevice(new class DiskDrive(this,"Drive.4",3));
```

Replace with:
```cpp
  sio->RegisterDevice(DriveChain[0] = new class DiskDrive(this,"Drive.1",0));
  sio->RegisterDevice(DriveChain[1] = new class DiskDrive(this,"Drive.2",1));
  sio->RegisterDevice(DriveChain[2] = new class DiskDrive(this,"Drive.3",2));
  sio->RegisterDevice(DriveChain[3] = new class DiskDrive(this,"Drive.4",3));
```

- [ ] **Step 5: Initialize DriveChain array in Machine constructor**

Find `Machine::Machine(void)` in machine.cpp. Add inside the constructor body:
```cpp
  DriveChain[0] = DriveChain[1] = DriveChain[2] = DriveChain[3] = NULL;
```

- [ ] **Step 6: Build to verify**

```bash
cd /home/t7jk/atari/atari++ && make -f makefile machine.o 2>&1 | grep -E "error:|warning:" | head -20
```
Expected: no errors.

- [ ] **Step 7: Commit**

```bash
cd /home/t7jk/atari && git add machine.hpp machine.cpp
git commit -m "feat: expose DriveOf(int) in Machine for indexed drive access"
```

---

## Task 3: Create `atarionline.hpp`

**Files:**
- Create: `atarionline.hpp`

- [ ] **Step 1: Write the header file**

Create `/home/t7jk/atari/atarionline.hpp`:

```cpp
/***********************************************************************************
 **
 ** Atari++ emulator (c) 2002 THOR-Software, Thomas Richter
 **
 ** In this module: In-emulator browser for the atarionline.pl Kaz catalog
 **********************************************************************************/

#ifndef ATARIONLINE_HPP
#define ATARIONLINE_HPP

/// Includes
#include "types.h"
#include "types.hpp"
#include "requester.hpp"
#include "list.hpp"
///

/// Forwards
class Machine;
class RenderPort;
class VerticalGroup;
class ButtonGadget;
///

/// Class AtariOnlineRequester
// An in-emulator browser for the atarionline.pl Kaz disk-image catalog.
// Navigation: Letters (level 0) → Game titles (level 1) → Files (level 2).
// Selected files are downloaded via curl to ~/.atari++/downloads/.
class AtariOnlineRequester : public Requester {
  //
  // A single navigable entry (letter, game title, or file).
  struct NavItem : public Node<struct NavItem> {
    enum Type { BACK, ENTRY, FILE } ItemType;
    char *DisplayName;   // human-readable label shown in list
    char *UrlParam;      // raw URL-encoded parameter value
    NavItem(void);
    ~NavItem(void);
  };
  //
  // Navigation state
  int   NavLevel;        // 0 = letters, 1 = game titles, 2 = files
  char *CurrentSub;      // URL-encoded letter, e.g. "B" (NULL at level 0)
  char *CurrentTitle;    // URL-encoded title, e.g. "+Bandit+Boulderdash" (NULL at levels 0-1)
  //
  // Local download directory (~/.atari++/downloads/)
  char  DownloadDir[512];
  //
  // Path of the downloaded file; set on success.
  char *SelectedFile;
  //
  // Gadget references (valid between BuildGadgets and CleanupGadgets only)
  class RenderPort    *ClipRegion;
  class VerticalGroup *Directory;
  class ButtonGadget  *CancelButton;
  //
  // Parsed navigation items for the current page
  List<struct NavItem>  Items;
  //
  // Custom return codes used between HandleEvent and the outer Request() loop
  enum AtariOnlineReturn {
    AOR_Navigate = RQ_Abort + 1,  // user clicked a folder/letter — reload page
    AOR_Download = RQ_Abort + 2   // file successfully downloaded
  };
  //
  // Build the fetch URL for the current navigation state
  void BuildUrl(char *buf, int bufsize) const;
  //
  // Fetch HTML via "curl -s URL"; caller must delete[] result. Returns NULL on failure.
  char *FetchHtml(const char *url) const;
  //
  // Parse letter links from level-0 page
  bool ParseLevel0(const char *html);
  // Parse game-title links from level-1 page
  bool ParseLevel1(const char *html);
  // Parse file links from level-2 page
  bool ParseLevel2(const char *html);
  //
  // Free all NavItem objects from Items list
  void ClearItems(void);
  //
  // Populate Directory VerticalGroup with RequesterEntry items from Items list
  void PopulateDirectoryGadgets(int w);
  //
  // Create DownloadDir if it does not exist. Returns false on failure.
  bool EnsureDownloadDir(void);
  //
  // Download file identified by URL-encoded fileParam to DownloadDir.
  // Sets SelectedFile on success. Returns false on failure.
  bool DownloadFile(const char *fileParam);
  //
  // Decode URL-encoding ('+' -> ' ', '%XX' -> char) into dst[dstsize].
  static void UrlDecode(char *dst, const char *src, int dstsize);
  //
  // Sanitize a decoded filename for use as a local filesystem name.
  // Replaces shell-unsafe characters with '_'.
  static void SanitizeFilename(char *name);
  //
  // Required overrides from Requester
  virtual void BuildGadgets(List<Gadget> &glist, class RenderPort *rport);
  virtual void CleanupGadgets(void);
  virtual int  HandleEvent(struct Event &event);
  //
public:
  AtariOnlineRequester(class Machine *mach);
  virtual ~AtariOnlineRequester(void);
  //
  // Open the browser. Returns non-zero if a file was successfully downloaded.
  // Overrides Requester::Request() to support multi-page navigation.
  int Request(void);
  //
  // After a successful Request(), returns the local path of the downloaded file.
  const char *SelectedItem(void) const
  {
    return SelectedFile;
  }
};
///

///
#endif
```

- [ ] **Step 2: Add atarionline to Makefile.atari FILES list**

In `Makefile.atari`, find the line containing `filerequester`:
```
			choicerequester filerequester \
```
Change it to:
```
			choicerequester filerequester atarionline \
```

- [ ] **Step 3: Create a stub atarionline.cpp to verify it compiles**

Create `/home/t7jk/atari/atarionline.cpp` with just the include and minimal stubs:

```cpp
#include "types.h"
#include "types.hpp"
#include "new.hpp"
#include "atarionline.hpp"
#include "machine.hpp"
#include "renderport.hpp"
#include "verticalgroup.hpp"
#include "buttongadget.hpp"
#include "textgadget.hpp"
#include "requesterentry.hpp"
#include "exceptions.hpp"
#include "event.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

AtariOnlineRequester::NavItem::NavItem(void)
  : DisplayName(NULL), UrlParam(NULL)
{}

AtariOnlineRequester::NavItem::~NavItem(void)
{
  delete[] DisplayName;
  delete[] UrlParam;
}

AtariOnlineRequester::AtariOnlineRequester(class Machine *mach)
  : Requester(mach), NavLevel(0), CurrentSub(NULL), CurrentTitle(NULL),
    SelectedFile(NULL), ClipRegion(NULL), Directory(NULL), CancelButton(NULL)
{
  DownloadDir[0] = '\0';
}

AtariOnlineRequester::~AtariOnlineRequester(void)
{
  delete[] CurrentSub;
  delete[] CurrentTitle;
  delete[] SelectedFile;
  ClearItems();
}

void AtariOnlineRequester::ClearItems(void)
{
  NavItem *it;
  while ((it = Items.First())) {
    it->Remove();
    delete it;
  }
}

void AtariOnlineRequester::BuildGadgets(List<Gadget> &, class RenderPort *)
{}

void AtariOnlineRequester::CleanupGadgets(void)
{
  delete ClipRegion;
  ClipRegion   = NULL;
  Directory    = NULL;
  CancelButton = NULL;
}

int AtariOnlineRequester::HandleEvent(struct Event &)
{
  return RQ_Abort;
}

int AtariOnlineRequester::Request(void)
{
  return 0;
}

void AtariOnlineRequester::BuildUrl(char *buf, int bufsize) const
{
  snprintf(buf, bufsize, "https://atarionline.pl/v01/index.php?ct=kazip");
}

char *AtariOnlineRequester::FetchHtml(const char *) const { return NULL; }
bool AtariOnlineRequester::ParseLevel0(const char *) { return false; }
bool AtariOnlineRequester::ParseLevel1(const char *) { return false; }
bool AtariOnlineRequester::ParseLevel2(const char *) { return false; }
bool AtariOnlineRequester::EnsureDownloadDir(void) { return false; }
bool AtariOnlineRequester::DownloadFile(const char *) { return false; }
void AtariOnlineRequester::UrlDecode(char *, const char *, int) {}
void AtariOnlineRequester::SanitizeFilename(char *) {}
void AtariOnlineRequester::PopulateDirectoryGadgets(int) {}
```

- [ ] **Step 4: Build stub to verify header compiles clean**

```bash
cd /home/t7jk/atari/atari++ && make -f makefile atarionline.o 2>&1 | grep -E "error:" | head -20
```
Expected: no errors.

- [ ] **Step 5: Commit**

```bash
cd /home/t7jk/atari && git add atarionline.hpp atarionline.cpp Makefile.atari
git commit -m "feat: scaffold AtariOnlineRequester class (stub implementation)"
```

---

## Task 4: Implement HTML Fetching and Parsing

**Files:**
- Modify: `atarionline.cpp` — replace stub bodies for FetchHtml, ParseLevel0/1/2, UrlDecode, SanitizeFilename, EnsureDownloadDir, BuildUrl

- [ ] **Step 1: Replace BuildUrl**

Replace the stub `BuildUrl`:
```cpp
void AtariOnlineRequester::BuildUrl(char *buf, int bufsize) const
{
  static const char BASE[] = "https://atarionline.pl/v01/index.php?ct=kazip";
  if (NavLevel == 0) {
    snprintf(buf, bufsize, "%s", BASE);
  } else if (NavLevel == 1) {
    snprintf(buf, bufsize, "%s&sub=%s", BASE, CurrentSub);
  } else {
    snprintf(buf, bufsize, "%s&sub=%s&title=%s", BASE, CurrentSub, CurrentTitle);
  }
}
```

- [ ] **Step 2: Replace UrlDecode**

```cpp
void AtariOnlineRequester::UrlDecode(char *dst, const char *src, int dstsize)
{
  int di = 0;
  for (const char *s = src; *s && di < dstsize - 1; s++) {
    if (*s == '+') {
      dst[di++] = ' ';
    } else if (*s == '%' && s[1] && s[2]) {
      char hex[3] = {s[1], s[2], '\0'};
      dst[di++] = (char)strtol(hex, NULL, 16);
      s += 2;
    } else {
      dst[di++] = *s;
    }
  }
  dst[di] = '\0';
}
```

- [ ] **Step 3: Replace SanitizeFilename**

```cpp
void AtariOnlineRequester::SanitizeFilename(char *name)
{
  for (char *c = name; *c; c++) {
    if (*c == '\'' || *c == '"' || *c == '`' || *c == '\\')
      *c = '_';
  }
}
```

- [ ] **Step 4: Replace FetchHtml**

```cpp
char *AtariOnlineRequester::FetchHtml(const char *url) const
{
  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "curl -s --max-time 15 '%s'", url);
  FILE *f = popen(cmd, "r");
  if (!f) return NULL;

  size_t capacity = 65536, size = 0;
  char  *buf      = new char[capacity];

  while (!feof(f)) {
    size_t n = fread(buf + size, 1, capacity - size - 1, f);
    if (n == 0) break;
    size += n;
    if (size >= capacity - 1) {
      char *nb = new char[capacity * 2];
      memcpy(nb, buf, size);
      delete[] buf;
      buf      = nb;
      capacity *= 2;
    }
  }
  pclose(f);
  buf[size] = '\0';
  if (size == 0) {
    delete[] buf;
    return NULL;
  }
  return buf;
}
```

- [ ] **Step 5: Replace ParseLevel0**

Extracts letters like "0-9", "A", "B", … from the index page.

```cpp
bool AtariOnlineRequester::ParseLevel0(const char *html)
{
  static const char NEEDLE[] = "href=\"index.php?ct=kazip&amp;sub=";
  static const int  NLEN     = (int)sizeof(NEEDLE) - 1;
  const char *p = html;
  bool found = false;

  while ((p = strstr(p, NEEDLE)) != NULL) {
    p += NLEN;
    const char *end = strchr(p, '"');
    if (!end) break;

    // Skip links that have further parameters (e.g. &amp;title=)
    bool hasamp = false;
    for (const char *q = p; q < end; q++) {
      if (*q == '&') { hasamp = true; break; }
    }
    if (hasamp) { p = end; continue; }

    int len = (int)(end - p);
    if (len <= 0 || len > 64) { p = end; continue; }

    char param[65];
    memcpy(param, p, len);
    param[len] = '\0';

    // Deduplicate
    bool dup = false;
    for (NavItem *it = Items.First(); it; it = it->NextOf()) {
      if (strcmp(it->UrlParam, param) == 0) { dup = true; break; }
    }
    if (dup) { p = end; continue; }

    char display[128];
    UrlDecode(display, param, sizeof(display));

    NavItem *item   = new NavItem;
    item->ItemType  = NavItem::ENTRY;
    item->UrlParam  = new char[len + 1];
    memcpy(item->UrlParam, param, len + 1);
    item->DisplayName = new char[strlen(display) + 1];
    strcpy(item->DisplayName, display);
    Items.AddTail(item);
    found = true;
    p = end;
  }
  return found;
}
```

- [ ] **Step 6: Replace ParseLevel1**

Extracts game title folder links for the current letter.

```cpp
bool AtariOnlineRequester::ParseLevel1(const char *html)
{
  char needle[256];
  snprintf(needle, sizeof(needle),
    "href=\"index.php?ct=kazip&amp;sub=%s&amp;title=", CurrentSub);
  int nlen = (int)strlen(needle);
  const char *p = html;
  bool found = false;

  while ((p = strstr(p, needle)) != NULL) {
    p += nlen;
    // Title ends at '#' (anchor) or '"'
    const char *end = p;
    while (*end && *end != '#' && *end != '"') end++;
    if (!*end) break;

    int len = (int)(end - p);
    if (len <= 0 || len > 256) { p = end; continue; }

    char param[257];
    memcpy(param, p, len);
    param[len] = '\0';

    // Deduplicate
    bool dup = false;
    for (NavItem *it = Items.First(); it; it = it->NextOf()) {
      if (strcmp(it->UrlParam, param) == 0) { dup = true; break; }
    }
    if (dup) { p = end; continue; }

    char display[256];
    UrlDecode(display, param, sizeof(display));
    // Strip leading spaces (URL title often starts with '+' -> ' ')
    const char *dstart = display;
    while (*dstart == ' ') dstart++;

    NavItem *item   = new NavItem;
    item->ItemType  = NavItem::ENTRY;
    item->UrlParam  = new char[len + 1];
    memcpy(item->UrlParam, param, len + 1);
    item->DisplayName = new char[strlen(dstart) + 1];
    strcpy(item->DisplayName, dstart);
    Items.AddTail(item);
    found = true;
    p = end;
  }
  return found;
}
```

- [ ] **Step 7: Replace ParseLevel2**

Extracts downloadable file links with supported extensions.

```cpp
bool AtariOnlineRequester::ParseLevel2(const char *html)
{
  char needle[512];
  snprintf(needle, sizeof(needle),
    "href=\"kazip.php?ct=kazip&amp;sub=%s&amp;title=%s&amp;file=",
    CurrentSub, CurrentTitle);
  int nlen = (int)strlen(needle);

  static const char *EXTS[] = {
    ".atr",".xex",".cas",".car",".bin",".xfd",".dcm",
    ".ATR",".XEX",".CAS",".CAR",".BIN",".XFD",".DCM", NULL
  };

  const char *p = html;
  bool found = false;

  while ((p = strstr(p, needle)) != NULL) {
    p += nlen;
    const char *end = strchr(p, '"');
    if (!end) break;

    int len = (int)(end - p);
    if (len <= 0 || len > 256) { p = end; continue; }

    char param[257];
    memcpy(param, p, len);
    param[len] = '\0';

    char decoded[256];
    UrlDecode(decoded, param, sizeof(decoded));

    // Accept only known disk-image extensions
    bool ok = false;
    int  dlen = (int)strlen(decoded);
    for (int i = 0; EXTS[i]; i++) {
      int elen = (int)strlen(EXTS[i]);
      if (dlen > elen && strcmp(decoded + dlen - elen, EXTS[i]) == 0) {
        ok = true; break;
      }
    }
    if (!ok) { p = end; continue; }

    NavItem *item   = new NavItem;
    item->ItemType  = NavItem::FILE;
    item->UrlParam  = new char[len + 1];
    memcpy(item->UrlParam, param, len + 1);
    item->DisplayName = new char[strlen(decoded) + 1];
    strcpy(item->DisplayName, decoded);
    Items.AddTail(item);
    found = true;
    p = end;
  }
  return found;
}
```

- [ ] **Step 8: Replace EnsureDownloadDir**

```cpp
bool AtariOnlineRequester::EnsureDownloadDir(void)
{
  const char *home = getenv("HOME");
  if (!home) return false;

  char base[512];
  snprintf(base, sizeof(base), "%s/.atari++", home);
  mkdir(base, 0755);  // ignore error if exists

  snprintf(DownloadDir, sizeof(DownloadDir), "%s/.atari++/downloads", home);
  if (mkdir(DownloadDir, 0755) != 0) {
    struct stat st;
    if (stat(DownloadDir, &st) != 0 || !S_ISDIR(st.st_mode))
      return false;
  }
  return true;
}
```

- [ ] **Step 9: Build to verify**

```bash
cd /home/t7jk/atari/atari++ && make -f makefile atarionline.o 2>&1 | grep -E "error:" | head -20
```
Expected: no errors.

- [ ] **Step 10: Commit**

```bash
cd /home/t7jk/atari && git add atarionline.cpp
git commit -m "feat: implement HTML fetching and parsing for AtariOnline browser"
```

---

## Task 5: Implement UI, Download, and Navigation Loop

**Files:**
- Modify: `atarionline.cpp` — replace stub bodies for PopulateDirectoryGadgets, BuildGadgets, HandleEvent, DownloadFile, Request

- [ ] **Step 1: Replace DownloadFile**

```cpp
bool AtariOnlineRequester::DownloadFile(const char *fileParam)
{
  char decoded[256];
  UrlDecode(decoded, fileParam, sizeof(decoded));
  SanitizeFilename(decoded);

  char localpath[768];
  snprintf(localpath, sizeof(localpath), "%s/%s", DownloadDir, decoded);

  char url[1024];
  snprintf(url, sizeof(url),
    "https://atarionline.pl/v01/kazip.php?ct=kazip&sub=%s&title=%s&file=%s",
    CurrentSub, CurrentTitle, fileParam);

  char cmd[2048];
  snprintf(cmd, sizeof(cmd),
    "curl -L --max-time 60 -o '%s' '%s'", localpath, url);

  if (system(cmd) != 0) return false;

  struct stat st;
  if (stat(localpath, &st) != 0 || st.st_size == 0) return false;

  delete[] SelectedFile;
  SelectedFile = new char[strlen(localpath) + 1];
  strcpy(SelectedFile, localpath);
  return true;
}
```

- [ ] **Step 2: Replace PopulateDirectoryGadgets**

```cpp
void AtariOnlineRequester::PopulateDirectoryGadgets(int w)
{
  static const LONG ITEM_H = 8;
  LONG te    = 0;
  LONG clipH = ClipRegion->HeightOf();

  // "Back" entry at all levels except level 0
  if (NavLevel > 0) {
    new class RequesterEntry(*Directory, ClipRegion, 0, te, w, ITEM_H, "<< Back", false);
    te += ITEM_H;
  }

  // Navigation / file entries
  for (NavItem *it = Items.First(); it; it = it->NextOf()) {
    bool isdir = (it->ItemType == NavItem::ENTRY);
    new class RequesterEntry(*Directory, ClipRegion, 0, te, w, ITEM_H,
                             it->DisplayName, isdir);
    te += ITEM_H;
  }

  // Blank filler to fill remaining visible area
  while (te + ITEM_H <= clipH) {
    new class RequesterEntry(*Directory, ClipRegion, 0, te, w, ITEM_H, NULL, false);
    te += ITEM_H;
  }
}
```

- [ ] **Step 3: Replace BuildGadgets**

```cpp
void AtariOnlineRequester::BuildGadgets(List<Gadget> &glist, class RenderPort *rport)
{
  LONG w = rport->WidthOf();
  LONG h = rport->HeightOf();

  // Title text at top (12 px)
  new class TextGadget(glist, rport, 0, 0, w, 12, "AtariOnline - Kaz Catalog");

  // Scrollable list area: full width, between title and cancel button
  ClipRegion = new class RenderPort(rport, 0, 12, w, h - 24);
  Directory  = new class VerticalGroup(glist, ClipRegion, 0, 0, w, h - 24);

  // Cancel button at bottom
  CancelButton = new class ButtonGadget(glist, rport, 0, h - 12, 76, 12, "Cancel");

  // Fill list from current Items
  PopulateDirectoryGadgets((int)w);
}
```

- [ ] **Step 4: Replace HandleEvent**

```cpp
int AtariOnlineRequester::HandleEvent(struct Event &event)
{
  if (event.Type != Event::GadgetUp || !event.Object)
    return RQ_Nothing;

  // Cancel button
  if (event.Object == CancelButton)
    return RQ_Abort;

  class RequesterEntry *entry = (class RequesterEntry *)event.Object;
  const char *label = entry->GetStatus();
  if (!label) return RQ_Nothing;

  // Back navigation
  if (strcmp(label, "<< Back") == 0) {
    if (NavLevel == 2) {
      delete[] CurrentTitle;
      CurrentTitle = NULL;
      NavLevel = 1;
    } else if (NavLevel == 1) {
      delete[] CurrentSub;
      CurrentSub = NULL;
      NavLevel = 0;
    }
    return int(AOR_Navigate);
  }

  // Find matching NavItem by display name
  NavItem *found = NULL;
  for (NavItem *it = Items.First(); it; it = it->NextOf()) {
    if (strcmp(it->DisplayName, label) == 0) { found = it; break; }
  }
  if (!found) return RQ_Nothing;

  if (found->ItemType == NavItem::ENTRY) {
    if (NavLevel == 0) {
      delete[] CurrentSub;
      int len = (int)strlen(found->UrlParam);
      CurrentSub = new char[len + 1];
      strcpy(CurrentSub, found->UrlParam);
      NavLevel = 1;
    } else if (NavLevel == 1) {
      delete[] CurrentTitle;
      int len = (int)strlen(found->UrlParam);
      CurrentTitle = new char[len + 1];
      strcpy(CurrentTitle, found->UrlParam);
      NavLevel = 2;
    }
    return int(AOR_Navigate);
  }

  if (found->ItemType == NavItem::FILE) {
    if (DownloadFile(found->UrlParam))
      return int(AOR_Download);
    // Download failed — keep requester open so user can retry/cancel
  }
  return RQ_Nothing;
}
```

- [ ] **Step 5: Replace Request()**

```cpp
int AtariOnlineRequester::Request(void)
{
  NavLevel = 0;
  delete[] CurrentSub;   CurrentSub   = NULL;
  delete[] CurrentTitle; CurrentTitle = NULL;
  delete[] SelectedFile; SelectedFile = NULL;

  if (!EnsureDownloadDir()) {
    MachineOf()->PutWarning(
      "AtariOnline: Cannot create ~/.atari++/downloads/ — check HOME environment variable");
    return 0;
  }

  for (;;) {
    ClearItems();

    char url[512];
    BuildUrl(url, sizeof(url));

    char *html = FetchHtml(url);
    if (!html) {
      MachineOf()->PutWarning("AtariOnline: Could not fetch page — check network connection");
      return 0;
    }

    bool parsed = false;
    if      (NavLevel == 0) parsed = ParseLevel0(html);
    else if (NavLevel == 1) parsed = ParseLevel1(html);
    else                    parsed = ParseLevel2(html);
    delete[] html;

    if (!parsed) {
      MachineOf()->PutWarning("AtariOnline: No entries found on this page");
      return 0;
    }

    int result = Requester::Request();

    if (result == int(AOR_Navigate)) continue;
    if (result == int(AOR_Download)) return 1;
    return 0;  // RQ_Abort: user cancelled
  }
}
```

- [ ] **Step 6: Build full binary to verify**

```bash
cd /home/t7jk/atari/atari++ && make -f makefile atari 2>&1 | grep -E "error:" | head -30
```
Expected: clean build, binary `atari++` produced.

- [ ] **Step 7: Commit**

```bash
cd /home/t7jk/atari && git add atarionline.cpp
git commit -m "feat: implement AtariOnlineRequester UI, navigation loop, and download"
```

---

## Task 6: Wire Up TitleMenu

**Files:**
- Modify: `titlemenu.hpp`
- Modify: `titlemenu.cpp`

- [ ] **Step 1: Add TA_AtariOnline to titlemenu.hpp enum**

Find the `enum ActionId` in titlemenu.hpp:
```cpp
    TA_Quit        // leave the emulator
  };
```
Change to:
```cpp
    TA_Quit,       // leave the emulator
    TA_AtariOnline // browse atarionline.pl and mount a disk
  };
```

- [ ] **Step 2: Add AtariOnlineReq member to TitleMenu**

In titlemenu.hpp, find the private members section. After:
```cpp
  class FileRequester *Requester;
```
Add:
```cpp
  class AtariOnlineRequester *AtariOnlineReq;
```

- [ ] **Step 3: Add forward declaration in titlemenu.hpp**

Find the Forwards block near the top of titlemenu.hpp:
```cpp
/// Forwards
class Machine;
```
Add `class AtariOnlineRequester;` to it:
```cpp
/// Forwards
class Machine;
class AtariOnlineRequester;
```

- [ ] **Step 4: Add #include in titlemenu.cpp**

Find the includes block in titlemenu.cpp. After:
```cpp
#include "filerequester.hpp"
```
Add:
```cpp
#include "atarionline.hpp"
```

- [ ] **Step 5: Construct AtariOnlineReq in TitleMenu constructor**

Find the constructor initializer list in titlemenu.cpp:
```cpp
    Requester(new class FileRequester(mach)),
```
Add after it:
```cpp
    AtariOnlineReq(new class AtariOnlineRequester(mach)),
```

- [ ] **Step 6: Destroy AtariOnlineReq in TitleMenu destructor**

Find `TitleMenu::~TitleMenu`:
```cpp
  delete Requester;
```
Add after it:
```cpp
  delete AtariOnlineReq;
```

- [ ] **Step 7: Add menu item in CollectTopics**

Find in `TitleMenu::CollectTopics`, the separator before "Exit":
```cpp
  new class MenuActionItem(projectmenu,"Exit",TA_Quit);
```
Insert a separator and the new item before it:
```cpp
  new class MenuSeparatorItem(projectmenu);
  new class MenuActionItem(projectmenu,"AtariOnline...",TA_AtariOnline);
  new class MenuActionItem(projectmenu,"Exit",TA_Quit);
```

- [ ] **Step 8: Add case handler in EnterMenu**

Find the switch in `TitleMenu::EnterMenu`. After the `TA_SaveState` case:
```cpp
      case TA_SaveState:
        SaveState();
        quit = true;
        break;
      default:
```
Add before `default:`:
```cpp
      case TA_AtariOnline:
        RemoveMenu();
        if (AtariOnlineReq->Request()) {
          if (Machine->DriveOf(0)) {
            Machine->DriveOf(0)->MountImage(AtariOnlineReq->SelectedItem());
          }
          Machine->ColdStart();
        }
        quit = true;
        break;
```

- [ ] **Step 9: Build full binary**

```bash
cd /home/t7jk/atari/atari++ && make -f makefile atari 2>&1 | grep -E "error:" | head -30
```
Expected: clean build.

- [ ] **Step 10: Commit**

```bash
cd /home/t7jk/atari && git add titlemenu.hpp titlemenu.cpp
git commit -m "feat: add AtariOnline menu entry to TitleMenu with drive mount and cold-start"
```

---

## Task 7: Integration Smoke Test

- [ ] **Step 1: Run the emulator**

```bash
cd /home/t7jk/atari/atari++ && ./atari++
```

- [ ] **Step 2: Open the title menu**

Move mouse to top edge of emulator window. The title-bar menu should appear.

- [ ] **Step 3: Verify AtariOnline entry exists**

In the "Project" menu, confirm "AtariOnline..." item is visible.

- [ ] **Step 4: Test level-0 navigation (letters)**

Click "AtariOnline...". The requester should open and show a list of letters: `0-9`, `A`, `B`, `C`, … `Z`.

- [ ] **Step 5: Test level-1 navigation (game titles)**

Click "B". The list should reload showing ~710 game titles starting with "B-1 Nuclear Bomber", "B.A.S.I.C. Adventure, The", etc.

- [ ] **Step 6: Test level-2 navigation (files)**

Click "Bandit Boulderdash". The list should show:
- `Bandit Boulderdash (v1).xex`
- `Bandit Boulderdash.atr`
- `Boulder Dash - Bandit (v2).xex`

- [ ] **Step 7: Test download and mount**

Click `Bandit Boulderdash.atr`. The file should download (curl output may be visible in terminal). Emulator should cold-start and boot the disk.

Verify:
```bash
ls -la ~/.atari++/downloads/Bandit\ Boulderdash.atr
```
File should exist and be non-zero.

- [ ] **Step 8: Test Back navigation**

Re-open AtariOnline, navigate into a letter, then click "<< Back". Should return to letter list.

- [ ] **Step 9: Test Cancel**

Re-open AtariOnline, click "Cancel". Should return to emulation without changes.

- [ ] **Step 10: Final commit with push**

```bash
cd /home/t7jk/atari && git push origin master
```

---

## Self-Review Notes

- All spec requirements covered: 3-level navigation ✓, curl HTTP ✓, `~/.atari++/downloads/` ✓, Drive 1 mount ✓, ColdStart ✓, Back navigation ✓, Cancel ✓, error messages via PutWarning ✓, file extension filter ✓
- No TBDs or placeholders in code blocks
- `MountImage` defined in Task 1 used in Task 6 ✓
- `DriveOf(int)` defined in Task 2 used in Task 6 ✓
- `AtariOnlineRequester::Request()` defined in Task 5 called in Task 6 ✓
- `SelectedItem()` defined in header (Task 3) used in Task 6 ✓
- `AOR_Navigate`/`AOR_Download` enum defined in header (Task 3) used in Task 5 ✓
