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
