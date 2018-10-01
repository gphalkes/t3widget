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

namespace t3widget {

class menu_item_base_t;
class menu_item_t;

class T3_WIDGET_API menu_panel_t : public dialog_t, public mouse_target_t {
  friend class menu_bar_t;
  friend class menu_item_t;

 private:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  void close() override;
  void set_menu_bar(menu_bar_t *_menu_bar);
  void draw_label(t3window::window_t *draw_window, t3_attr_t attr, bool selected) const;
  int get_label_width() const;
  bool is_hotkey(key_t key) const;
  void activate(int idx);
  void recompute_panel_dimensions();

 protected:
  bool is_child(const window_component_t *widget) const override;

  /* Process the mouse event. If this menu_panel_t is associated with a menu_bar_t, the events will
     be preprocessed by the menu_bar_t, and EMOUSE_OUTSIDE_GRAB will never be included. */
  bool process_mouse_event(mouse_event_t event) override;

  void hide() override;

 public:
  menu_panel_t(string_view name);
  ~menu_panel_t() override;
  bool process_key(key_t key) override;
  void set_position(optint top, optint left) override;
  bool set_size(optint height, optint width) override;

  /** Constructs a new #menu_item_t and adds it to the menu before the specified item.
      @param label The #smart_label_t text of the menu item.
      @param shortcut_key The text of the shortcut key.
      @param id The id to associate with this menu item, which will be passed in the callback.
      @param before The menu item to insert the new item before, or @c nullptr for append.
      @returns a non-owning pointer to the newly created item. */
  menu_item_base_t *insert_item(string_view label, string_view shortcut_key, int id,
                                const menu_item_base_t *before = nullptr);

  /** Adds the given item to the menu.
      @param before The menu item to insert the new item before, or @c nullptr for append.
      @returns a non-owning pointer to the added item. */
  menu_item_base_t *insert_item(std::unique_ptr<menu_item_t> item,
                                const menu_item_base_t *before = nullptr);
  /** Constructs a new #menu_separator_t and adds it to the menu.
      @param before The menu item to insert the new separator before, or @c nullptr for append.
      @returns a non-owning pointer to the newly constructed separator. */
  menu_item_base_t *insert_separator(const menu_item_base_t *before = nullptr);

  /** Removes @p item from the menu, if it is part of the menu.
      @returns the removed item. */
  std::unique_ptr<menu_item_base_t> remove_item(menu_item_base_t *item);

  void show() override;

  connection_t connect_activate(std::function<void(int)> cb);
};

}  // namespace t3widget
#endif
