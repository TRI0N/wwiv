/**************************************************************************/
/*                                                                        */
/*                  WWIV Initialization Utility Version 5                 */
/*               Copyright (C)2014-2017, WWIV Software Services           */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/*                                                                        */
/**************************************************************************/
#include "init/paths.h"

#include <cstdint>
#include <memory>
#include <string>

#include "core/strings.h"
#include "core/wwivport.h"
#include "init/init.h"
#include "init/utility.h"
#include "init/wwivinit.h"
#include "localui/wwiv_curses.h"
#include "localui/input.h"

using std::unique_ptr;
using std::string;
using namespace wwiv::strings;

static const int COL1_LINE = 2;
static const int COL1_POSITION = 14;

/* change msgsdir, gfilesdir, datadir, dloadsdir, ramdrive, tempdir, scriptdir */
void setpaths(const std::string& bbsdir) {
  out->Cls(ACS_CKBOARD);
  unique_ptr<CursesWindow> window(out->CreateBoxedWindow("System Paths", 15, 76));

  int y = 1;
  window->PrintfXY(COL1_LINE, y++, "Messages  : %s", syscfg.msgsdir);
  window->PrintfXY(COL1_LINE, y++, "GFiles    : %s", syscfg.gfilesdir);
  window->PrintfXY(COL1_LINE, y++, "Menus     : %s", syscfg.menudir);
  window->PrintfXY(COL1_LINE, y++, "Data      : %s", syscfg.datadir);
  window->PrintfXY(COL1_LINE, y++, "Scripts   : %s", syscfg.scriptdir);
  window->PrintfXY(COL1_LINE, y++, "Downloads : %s", syscfg.dloadsdir);
  y+=2;
  window->SetColor(SchemeId::WARNING);
  window->PrintfXY(COL1_LINE, y++, "CAUTION: ONLY EXPERIENCED SYSOPS SHOULD MODIFY THESE SETTINGS.");
  y+=1;
  window->SetColor(SchemeId::WINDOW_TEXT);
  window->PrintfXY(COL1_LINE + 2, y++, "Changing any of these requires YOU to MANUALLY move files and/or");
  window->PrintfXY(COL1_LINE + 2, y++, "directory structures.");

  if (!syscfg.scriptdir[0]) {
    // This is added in 5.3
    string sdir = StrCat("scripts", File::pathSeparatorString);
    to_char_array(syscfg.scriptdir, sdir);
  }

  EditItems items{
    new FilePathItem(COL1_POSITION, 1, 60, bbsdir, syscfg.msgsdir),
    new FilePathItem(COL1_POSITION, 2, 60, bbsdir, syscfg.gfilesdir),
    new FilePathItem(COL1_POSITION, 3, 60, bbsdir, syscfg.menudir),
    new FilePathItem(COL1_POSITION, 4, 60, bbsdir, syscfg.datadir),
    new FilePathItem(COL1_POSITION, 5, 60, bbsdir, syscfg.scriptdir),
    new FilePathItem(COL1_POSITION, 6, 60, bbsdir, syscfg.dloadsdir),
  };

  items.set_curses_io(out, window.get());
  items.Run();

  save_config();
}

