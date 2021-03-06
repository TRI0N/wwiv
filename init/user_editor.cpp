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
#include "init/user_editor.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

#include "core/strings.h"
#include "core/datafile.h"
#include "core/file.h"
#include "init/init.h"
#include "init/utility.h"
#include "init/wwivinit.h"
#include "localui/wwiv_curses.h"
#include "localui/input.h"
#include "localui/listbox.h"
#include "sdk/config.h"
#include "sdk/filenames.h"
#include "sdk/user.h"
#include "sdk/usermanager.h"

static const int COL1_POSITION = 17;
static const int COL2_POSITION = 50;
static const int COL1_LINE = 2;

using std::string;
using std::unique_ptr;
using std::vector;
using namespace wwiv::core;
using namespace wwiv::strings;

static bool IsUserDeleted(userrec *user) {
  return user->inact & inact_deleted;
}

static void show_user(EditItems* items, userrec* user) {
  items->Display();

  items->window()->SetColor(SchemeId::WINDOW_TEXT);
  auto height = items->window()->GetMaxY() - 2;
  auto width = items->window()->GetMaxX() - 2;
  const string blank(width - COL2_POSITION, ' ');
  items->window()->SetColor(SchemeId::WINDOW_TEXT);
  for (int i = 1; i < height; i++) {
    items->window()->PutsXY(COL2_POSITION, i, blank);
  }
  if (user->inact & inact_deleted) {
    items->window()->SetColor(SchemeId::ERROR_TEXT);
    items->window()->PutsXY(COL2_POSITION, 1, "[[ DELETED USER ]] ");
  } else if (user->inact & inact_inactive) {
    items->window()->SetColor(SchemeId::ERROR_TEXT);
    items->window()->PutsXY(COL2_POSITION, 1, "[[ INACTIVE USER ]]");
  }
  items->window()->SetColor(SchemeId::WINDOW_TEXT);
  int y = 2;
  items->window()->PrintfXY(COL2_POSITION, y++, "First on     : %s", user->firston);
  items->window()->PrintfXY(COL2_POSITION, y++, "Last on      : %s", user->laston);
  y++;
  items->window()->PrintfXY(COL2_POSITION, y++, "Total Calls  : %d", user->logons);
  items->window()->PrintfXY(COL2_POSITION, y++, "Today Calls  : %d", user->ontoday);
  items->window()->PrintfXY(COL2_POSITION, y++, "Bad Logins   : %d", user->illegal);
  y++;
  items->window()->PrintfXY(COL2_POSITION, y++, "Num of Posts : %d", user->msgpost);
  items->window()->PrintfXY(COL2_POSITION, y++, "Num of Emails: %d", user->emailsent);
  items->window()->PrintfXY(COL2_POSITION, y++, "Feedback Sent: %d", user->feedbacksent);
  items->window()->PrintfXY(COL2_POSITION, y++, "Msgs Waiting : %d", user->waiting);
  items->window()->PrintfXY(COL2_POSITION, y++, "Netmail Sent : %d", user->emailnet);
  items->window()->PrintfXY(COL2_POSITION, y++, "Deleted Posts: %d", user->deletedposts);
}

static void show_error_no_users(CursesWindow* window) {
  messagebox(window, "You must have users added before using user editor.");
}

static vector<HelpItem> create_extra_help_items() {
  vector<HelpItem> help_items = { { "D", "Delete" }, { "J", "Jump" }, { "R", "Restore" } };
  return help_items;
}

static const int JumpToUser(CursesWindow* window, const std::string& datadir) {
  vector<ListBoxItem> items;
  {
    DataFile<smalrec> file(datadir, NAMES_LST, File::modeReadOnly | File::modeBinary, File::shareDenyWrite);
    if (!file) {
      show_error_no_users(window);
      return -1;
    }

    const int num_records = file.number_of_records();
    for (int i = 0; i < num_records; i++) {
      smalrec name;
      if (!file.Read(&name)) {
        messagebox(window, "Error reading smalrec");
        return -1;
      }
      items.emplace_back(StringPrintf("%s #%d", name.name, name.number), 0, name.number);
    }
  }
  
  ListBox list(out, window, "Select User", static_cast<int>(floor(window->GetMaxX() * 0.8)), 
    static_cast<int>(floor(window->GetMaxY() * 0.8)), items, out->color_scheme());
  ListBoxResult result = list.Run();
  if (result.type == ListBoxResultType::SELECTION) {
    return items[result.selected].data();
  }
  return -1;
}

void user_editor(const wwiv::sdk::Config& config) {
  int number_users = number_userrecs(config.datadir());
  out->Cls(ACS_CKBOARD);
  unique_ptr<CursesWindow> window(out->CreateBoxedWindow("User Editor", 18, 76));

  if (number_users < 1) {
    show_error_no_users(window.get());
    return;
  }

  int y = 1;
  window->PrintfXY(COL1_LINE, y++, "Name/Handle  :");
  window->PrintfXY(COL1_LINE, y++, "Real Name    :");
  window->PrintfXY(COL1_LINE, y++, "SL           :");
  window->PrintfXY(COL1_LINE, y++, "DSL          :");
  window->PrintfXY(COL1_LINE, y++, "Address      :");
  window->PrintfXY(COL1_LINE, y++, "City         :");
  window->PrintfXY(COL1_LINE, y++, "State        :");
  window->PrintfXY(COL1_LINE, y++, "Postal Code  :");
  window->PrintfXY(COL1_LINE, y++, "Birthday     :");
  window->PrintfXY(COL1_LINE, y++, "Password     :");
  window->PrintfXY(COL1_LINE, y++, "Phone Number :");
  window->PrintfXY(COL1_LINE, y++, "Data Number  :");
  window->PrintfXY(COL1_LINE, y++, "Computer Type:");
  window->PrintfXY(COL1_LINE, y++, "Restrictions :");
  window->PrintfXY(COL1_LINE, y++, "WWIV Reg     :");
  window->PrintfXY(COL1_LINE, y++, "Sysop Note   :");
  window->Refresh();

  int current_usernum = 1;
  userrec user;
  read_user(config.datadir(), current_usernum, &user);

  auto user_name_field = new StringEditItem<unsigned char*>(COL1_POSITION, 1, 30, user.name, true);
  user_name_field->set_displayfn([&]() -> string {
    return StringPrintf("%s #%d", user.name, current_usernum);
  });

  auto birthday_field = new CustomEditItem(COL1_POSITION, 9, 10, 
      [&]() -> string { 
        return StringPrintf("%2.2d/%2.2d/%4.4d", user.month, user.day, user.year + 1900);
      },
      [&](const string& s) {
        if (s[2] != '/' || s[5] != '/') {
          return;
        }
        time_t t = time(nullptr);
        struct tm * pTm = localtime(&t);
        int current_year = pTm->tm_year+1900;
        uint8_t month = to_number<uint8_t>(s.substr(0, 2));
        if (month < 1 || month > 12) { return; }
        uint8_t day = to_number<uint8_t>(s.substr(3, 2));
        if (day < 1 || day > 31) { return; }
        int year = to_number<int>(s.substr(6, 4));
        if (year < 1900 || year > current_year) { return ; }

        user.month = month;
        user.day = day;
        user.year = static_cast<uint8_t>(year - 1900);
      });

  EditItems items{
    user_name_field,
    new StringEditItem<unsigned char*>(COL1_POSITION, 2, 20, user.realname, false),
    new NumberEditItem<uint8_t>(COL1_POSITION, 3, &user.sl),
    new NumberEditItem<uint8_t>(COL1_POSITION, 4, &user.dsl),
    new StringEditItem<char*>(COL1_POSITION, 5, 30, user.street, false),
    new StringEditItem<char*>(COL1_POSITION, 6, 30, user.city, false),
    new StringEditItem<char*>(COL1_POSITION, 7, 2, user.state, false),
    new StringEditItem<char*>(COL1_POSITION, 8, 10, user.zipcode, true),
    birthday_field,
    new StringEditItem<char*>(COL1_POSITION, 10, 8, user.pw, true),
    new StringEditItem<char*>(COL1_POSITION, 11, 12, user.phone, true),
    new StringEditItem<char*>(COL1_POSITION, 12, 12, user.dataphone, true),
    new NumberEditItem<int8_t>(COL1_POSITION, 13, &user.comp_type),
    new RestrictionsEditItem(COL1_POSITION, 14, &user.restrict),
    new NumberEditItem<uint32_t>(COL1_POSITION, 15, &user.wwiv_regnum),
    new StringEditItem<char*>(COL1_POSITION, 16, 57, user.note, false),
  };
  items.set_navigation_extra_help_items(create_extra_help_items());
  items.set_curses_io(out, window.get());

  show_user(&items, &user);

  for (;;)  {
    char ch = onek(window.get(), "\033DJRQ[]{}\r");
    switch (ch) {
    case '\r': {
      if (IsUserDeleted(&user)) {
        window->SetColor(SchemeId::ERROR_TEXT);
        messagebox(window.get(), "Can not edit a deleted user.");
      } else {
        items.Run();
        if (dialog_yn(window.get(), "Save User?")) {
          write_user(config.datadir(), current_usernum, &user);
        }
      }
      window->Refresh();
    } break;
    case 'D': {
      // Delete user.
      wwiv::sdk::User u(user);
      if (u.IsUserDeleted()) {
        break;
      }
      if (!dialog_yn(window.get(), StrCat("Are you sure you want to delete ", u.GetName(), "? "))) {
        break;
      }
      wwiv::sdk::UserManager um(config);
      if (!um.delete_user(current_usernum)) {
        messagebox(window.get(), "Error trying to restore user.");
      }
    } break;
    case 'J': {
      int user_number = JumpToUser(window.get(), config.datadir());
      if (user_number >= 1) {
        current_usernum = user_number;
      }
    } break;
    case 'R': {
      // Restore Deleted User.
      wwiv::sdk::User u(user);
      if (!u.IsUserDeleted()) {
        break;
      }
      wwiv::sdk::UserManager um(config);
      if (!um.restore_user(current_usernum)) {
        messagebox(window.get(), "Error trying to restore user.");
      }
    } break;
    case 'Q':
    case '\033':
      return;
    case ']':
      if (++current_usernum > number_users) {
        current_usernum = 1;
      }
      break;
    case '[': {
      if (--current_usernum < 1) {
        current_usernum = number_users;
      }
    } break;
    case '}':
      current_usernum += 10;
      if (current_usernum > number_users) {
        current_usernum = number_users;
      }
      break;
    case '{':
      current_usernum -= 10;
      if (current_usernum < 1) {
        current_usernum = 1;
      }
      break;
    }

    read_user(config.datadir(), current_usernum, &user);
    show_user(&items, &user);
  }
}

