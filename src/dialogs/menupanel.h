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
#ifndef T3_WIDGET_MENUPANEL_H
#define T3_WIDGET_MENUPANEL_H

#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/menu.h>

namespace t3_widget {

class menu_item_base_t;
class menu_item_t;

class T3_WIDGET_API menu_panel_t : public dialog_t, public mouse_target_t {
  friend class menu_bar_t;
  friend class menu_item_t;

 private:
  struct T3_WIDGET_LOCAL implementation_t {
    int width, label_width, hotkey_width;
    smart_label_text_t label;
    menu_bar_t *menu_bar;

    implementation_t(const char *name) : label(name), menu_bar(nullptr) {}
  };
  std::unique_ptr<implementation_t> impl;

  void close() override;
  void set_menu_bar(menu_bar_t *_menu_bar);
  void draw_label(window_wrapper_t *draw_window, t3_attr_t attr, bool selected) const;
  int get_label_width() const;
  bool is_hotkey(key_t key) const;

 protected:
  bool is_child(window_component_t *widget) override;

  /* Process the mouse event. If this menu_panel_t is associated with a menu_bar_t, the events will
     be preprocessed by the menu_bar_t, and EMOUSE_OUTSIDE_GRAB will never be included. */
  bool process_mouse_event(mouse_event_t event) override;

  void hide() override;

 public:
  menu_panel_t(const char *name, menu_bar_t *_menu_bar = nullptr);
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;
  menu_item_base_t *add_item(const char *label, const char *hotkey, int id);
  menu_item_base_t *add_item(menu_item_t *item);
  menu_item_base_t *add_separator();
  void remove_item(menu_item_base_t *item);
  menu_item_base_t *replace_item(menu_item_base_t *old_item, const char *label, const char *hotkey,
                                 int id);
  menu_item_base_t *replace_item(menu_item_base_t *old_item, menu_item_t *new_item);

  void show() override;

  T3_WIDGET_SIGNAL(activate, void, int);
};

}  // namespace
#endif
