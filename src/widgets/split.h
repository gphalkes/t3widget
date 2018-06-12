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
#ifndef T3_WIDGET_SPLIT_H
#define T3_WIDGET_SPLIT_H

#include <map>
#include <string>
#include <vector>

#include <t3widget/key_binding.h>
#include <t3widget/widgets/widget.h>

namespace t3widget {

/** A widget that can contain multiple other widgets.

    The space allocated to a split_t widget can be divided either horizontally
    or vertically. When a horizontally split split_t is asked to split vertically,
    it will in fact create a new sub split_t which divides its space vertically.
    This widget is mostly useful to divide an area over widgets like edit_window_t
    or text_window_t.
*/
class T3_WIDGET_API split_t : public widget_t, public container_t {
 protected:
  struct T3_WIDGET_LOCAL implementation_t;
  single_alloc_pimpl_t<implementation_t> impl;

  /** Make the next widget the current widget (internal).
      Because split_t widgets may be nested, calling #next on this widget
      may actually need to tell a nested split_t to activate the next widget.
      This needs slightly different handling than the generic #next call,
      and needs to report whether the end of the list was reached. In the
      latter case, the return value shall be @c false.
  */
  bool next_recurse();
  /** Make the previous widget the current widget (internal).
      See #next_recurse for details.
  */
  bool previous_recurse();
  /** Make the first widget the active widget. */
  void set_to_begin();
  /** Make the last widget the active widget. */
  void set_to_end();
  /** Remove the currently activated widget from the split_t (internal).
      This function takes into account that split_t widgets may be nested,
      and if it finds the the widget to remove is in fact a split_t it will
      ask that to remove its current widget instead. If that leaves only
      a single widget in the nested split_t, this funciton will return
      @c true and the remaining widget in the nested split_t will replace
      the nested split_t in this widget. */
  bool unsplit(std::unique_ptr<widget_t> *widget);

 public:
  /** Create a new split_t. */
  split_t(std::unique_ptr<widget_t> widget);
  /** Destroy a split_t.
      Deletes all contained widgets as well.
  */
  ~split_t() override;
  bool process_key(key_t key) override;
  bool set_size(optint height, optint width) override;
  void update_contents() override;
  void set_focus(focus_t focus) override;
  void force_redraw() override;
  void set_child_focus(window_component_t *target) override;
  bool is_child(window_component_t *component) override;

  /** Divide the available screen space over one more widget.
      @param widget The widget to add.
      @param _horizontal Whether to split the space horizontally or vertically.

      This function may create a new nested split_t, if the current split_t
      is split horizontally and a vertical split is requested or vice versa.
      Note that if the current split_t is already a nested split_t, it may
      nest even further.
  */
  void split(std::unique_ptr<widget_t> widget, bool _horizontal);
  /** Remove the current widget from the split_t (or a nested split_t).
      @return The widget that was removed.
  */
  std::unique_ptr<widget_t> unsplit();
  /** Make the next widget the active widget. */
  void next();
  /** Make the previous widget the active widget. */
  void previous();
  /** Get the currently active widget. */
  widget_t *get_current();

#define _T3_ACTION_FILE <t3widget/widgets/split.actions.h>
#include <t3widget/key_binding_decl.h>
#undef _T3_ACTION_FILE
};

}  // namespace t3widget
#endif
