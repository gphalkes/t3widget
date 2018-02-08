/* Copyright (C) 2011-2012,2018 G.P. Halkes
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
#ifndef T3_WIDGET_AUTOCOMPLETER_H
#define T3_WIDGET_AUTOCOMPLETER_H

#include <t3widget/contentlist.h>
#include <t3widget/textbuffer.h>

namespace t3_widget {

class T3_WIDGET_API autocompleter_t {
 public:
  virtual ~autocompleter_t();
  virtual string_list_base_t *build_autocomplete_list(const text_buffer_t *text, int *position) = 0;
  virtual void autocomplete(text_buffer_t *text, size_t idx) = 0;
};

};  // namespace

#endif
