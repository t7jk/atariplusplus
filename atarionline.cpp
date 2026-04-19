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
