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
#ifndef T3_WIDGET_CLIPBOARD_H
#define T3_WIDGET_CLIPBOARD_H

#include <t3widget/widget_api.h>
#include <t3widget/ptr.h>
#include <string>

#define WITH_CLIPBOARD_LOCK(code) { t3_widget::ensure_clipboard_lock_t _lock; code }

namespace t3_widget {

T3_WIDGET_API linked_ptr<std::string>::t get_clipboard(void);
T3_WIDGET_API linked_ptr<std::string>::t get_primary(void);

T3_WIDGET_API void set_clipboard(std::string *str);
T3_WIDGET_API void set_primary(std::string *str);
T3_WIDGET_API void release_selections(void);
T3_WIDGET_API void lock_clipboard(void);
T3_WIDGET_API void unlock_clipboard(void);

class T3_WIDGET_API ensure_clipboard_lock_t {
	public:
		ensure_clipboard_lock_t(void) { lock_clipboard(); }
		~ensure_clipboard_lock_t(void) { unlock_clipboard(); }
};

}; // namespace
#endif
