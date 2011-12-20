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
#ifndef T3_WIDGET_X11_H
#define T3_WIDGET_X11_H

#include <t3widget/widget_api.h>
#include <string>
#include <pthread.h>

namespace t3_widget {

T3_WIDGET_LOCAL extern linked_ptr<std::string> clipboard_data;
T3_WIDGET_LOCAL extern linked_ptr<std::string> primary_data;

T3_WIDGET_LOCAL bool init_x11(void);
T3_WIDGET_LOCAL bool x11_working(void);
T3_WIDGET_LOCAL linked_ptr<std::string> get_x11_selection(bool clipboard);
T3_WIDGET_LOCAL void claim_selection(bool clipboard, std::string *data);
T3_WIDGET_LOCAL void release_selections(void);

}; // namespace
#endif
