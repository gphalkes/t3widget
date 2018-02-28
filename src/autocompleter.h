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

/** Interface class for autocompleters. */
class T3_WIDGET_API autocompleter_t {
 public:
  virtual ~autocompleter_t();
  /** Called to request the list of possible completions, given the current text buffer.
      @param text The text buffer to autocomplete.
      @param position Return value for the starting position of the token being auto-completed.
      @returns The suggested completions, owned by the auto-completer.

      The request is for the word at the cursor position. The auto-completer decides where the token
      to be completed starts, and returns this in @p position. This is used only for positioning the
      pop-up window with suggestions. */
  virtual string_list_base_t *build_autocomplete_list(const text_buffer_t *text, int *position) = 0;

  /** Called to request the modification of the text buffer, given the selection of suggestion idx.
      @param text The text_buffer_t to modify.
      @param idx The selected suggestion index in the list returned earlier by a call to
          build_autocomplete_list. */
  virtual void autocomplete(text_buffer_t *text, size_t idx) = 0;
};

}  // namespace

#endif
