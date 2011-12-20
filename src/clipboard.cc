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
#include "clipboard.h"
#ifdef WITH_X11
#include "x11.h"
#endif
#include "ptr.h"
#include "log.h"

using namespace std;

namespace t3_widget {

linked_ptr<string> clipboard_data;
linked_ptr<string> primary_data;

linked_ptr<string> get_clipboard(void) {
#ifdef WITH_X11
	return get_x11_selection(true);
#else
	return clipboard_data;
#endif
}

linked_ptr<string> get_primary(void) {
#ifdef WITH_X11
	return get_x11_selection(false);
#else
	return primary_data;
#endif
}

void set_clipboard(string *str) {
#ifdef WITH_X11
	claim_selection(true, str);
#else
	clipboard_data = str;
#endif
}

void set_primary(string *str) {
#ifdef WITH_X11
	claim_selection(false, str);
#else
	primary_data = str;
#endif
}

}; // namespace
