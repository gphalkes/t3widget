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
#ifndef T3_WIDGET_SEPARATOR_H
#define T3_WIDGET_SEPARATOR_H

#include <t3widget/widgets/widget.h>

namespace t3widget {

class T3_WIDGET_API separator_t : public widget_t {
 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

 public:
  separator_t(bool horizontal = true);
  ~separator_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  bool accepts_focus() const override;
};

}  // namespace t3widget
#endif
