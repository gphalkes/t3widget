/* Copyright (C) 2011-2013,2018 G.P. Halkes
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
#ifndef T3_WIDGET_LISTPANE_H
#define T3_WIDGET_LISTPANE_H

#include <memory>

#include <t3widget/util.h>
#include <t3widget/widgets/scrollbar.h>
#include <t3widget/widgets/widget.h>

namespace t3widget {

class T3_WIDGET_API list_pane_t : public widget_t, public container_t {
 private:
  class T3_WIDGET_LOCAL indicator_widget_t;
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  void ensure_cursor_on_screen();
  void scroll(int change);
  void scrollbar_clicked(scrollbar_t::step_t step);
  void scrollbar_dragged(text_pos_t start);

 protected:
  bool set_widget_parent(window_component_t *widget) override;

 public:
  list_pane_t(bool _indicator);
  ~list_pane_t() override;
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  void set_anchor(window_component_t *anchor, int relation) override;
  void force_redraw() override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(const window_component_t *widget) const override;
  bool process_mouse_event(mouse_event_t event) override;
  void reset();
  void update_positions();

  void push_back(std::unique_ptr<widget_t> widget);
  void push_front(std::unique_ptr<widget_t> widget);
  std::unique_ptr<widget_t> pop_back();
  void pop_front();
  widget_t *back();
  widget_t *operator[](int idx);
  size_t size();
  bool empty();

  typedef size_t iterator;

  iterator erase(iterator position);
  iterator begin();
  iterator end();

  size_t get_current() const;
  void set_current(size_t idx);

  void set_single_click_activate(bool sca);

  connection_t connect_activate(std::function<void()> cb);
  connection_t connect_selection_changed(std::function<void()> cb);
};

}  // namespace t3widget
#endif
