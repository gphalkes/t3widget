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
#include "x11.h"
#include "ptr.h"
#include "log.h"

using namespace std;

namespace t3_widget {

static linked_ptr<string> clipboard;

linked_ptr<string> get_clipboard(void) {
	string *x11_result = get_x11_selection(true);
	lprintf("get_x11_selection result: %s\n", x11_result == NULL ? "(null)" : x11_result->c_str());
	if (x11_result != NULL)
		return x11_result;
	return clipboard;
}

linked_ptr<string> get_primary(void) {
	return NULL;
}

void set_clipboard(string *str) {
	clipboard = str;
}

void set_primary(string *str) { (void) str; }

}; // namespace
