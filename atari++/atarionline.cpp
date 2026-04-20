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
  static const char BASE[] = "https://atarionline.pl/v01/index.php?ct=kazip";
  if (NavLevel == 0) {
    snprintf(buf, bufsize, "%s", BASE);
  } else if (NavLevel == 1) {
    snprintf(buf, bufsize, "%s&sub=%s", BASE, CurrentSub);
  } else {
    snprintf(buf, bufsize, "%s&sub=%s&title=%s", BASE, CurrentSub, CurrentTitle);
  }
}

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
    const char *end = p;
    while (*end && *end != '#' && *end != '"') end++;
    if (!*end) break;

    int len = (int)(end - p);
    if (len <= 0 || len > 256) { p = end; continue; }

    char param[257];
    memcpy(param, p, len);
    param[len] = '\0';

    bool dup = false;
    for (NavItem *it = Items.First(); it; it = it->NextOf()) {
      if (strcmp(it->UrlParam, param) == 0) { dup = true; break; }
    }
    if (dup) { p = end; continue; }

    char display[256];
    UrlDecode(display, param, sizeof(display));
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
bool AtariOnlineRequester::EnsureDownloadDir(void)
{
  const char *home = getenv("HOME");
  if (!home) return false;

  char base[512];
  snprintf(base, sizeof(base), "%s/.atari++", home);
  mkdir(base, 0755);

  snprintf(DownloadDir, sizeof(DownloadDir), "%s/.atari++/downloads", home);
  if (mkdir(DownloadDir, 0755) != 0) {
    struct stat st;
    if (stat(DownloadDir, &st) != 0 || !S_ISDIR(st.st_mode))
      return false;
  }
  return true;
}
bool AtariOnlineRequester::DownloadFile(const char *) { return false; }
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
void AtariOnlineRequester::SanitizeFilename(char *name)
{
  for (char *c = name; *c; c++) {
    if (*c == '\'' || *c == '"' || *c == '`' || *c == '\\')
      *c = '_';
  }
}
void AtariOnlineRequester::PopulateDirectoryGadgets(int) {}
