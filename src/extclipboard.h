/* Copyright (C) 2011 G.P. Halkes
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3, as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef T3_WIDGET_EXTCLIPBOARD_H
#define T3_WIDGET_EXTCLIPBOARD_H

#ifndef _T3_WIDGET_INTERNAL
#error This header file is for internal use _only_!!
#endif

/* This file only contains the definition of the interface struct used to
   communicate with the X11 module. It should _not_ contain any symbol that is
   dependent on the X11 headers. */

#include "ptr.h"
#include "widget_api.h"
#include <string>

namespace t3_widget {

T3_WIDGET_API extern linked_ptr<std::string>::t clipboard_data;
T3_WIDGET_API extern linked_ptr<std::string>::t primary_data;

#define EXTCLIPBOARD_VERSION 1

struct extclipboard_interface_t {
  int version;
  bool (*init)();
  void (*release_selections)();
  linked_ptr<std::string>::t (*get_selection)(bool clipboard);
  void (*claim_selection)(bool clipboard, std::string *data);
  void (*lock)();
  void (*unlock)();
  void (*stop)();
};

};  // namespace
#endif
