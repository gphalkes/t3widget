/* Copyright (C) 2011-2013 G.P. Halkes
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
#ifndef T3_WIDGET_FILEPANE_H
#define T3_WIDGET_FILEPANE_H

#include <string>

#include <t3widget/dialogs/popup.h>
#include <t3widget/widgets/widget.h>
#include <t3widget/contentlist.h>
#include <t3widget/widgets/scrollbar.h>
#include <t3widget/widgets/textfield.h>

#define _T3_WDIGET_FP_MAX_COLUMNS 8

namespace t3_widget {

/** A widget displaying the contents of a directory. */
class T3_WIDGET_API file_pane_t : public widget_t, public container_t {
	private:
		class search_panel_t;

		struct T3_WIDGET_LOCAL implementation_t {
			scrollbar_t scrollbar; /**< Scrollbar displayed at the bottom. */
			size_t top_idx, /**< Index of the first item displayed. */
				current; /**< Index of the currently highlighted item. */
			file_list_t *file_list; /**< List of files to display. */
			bool focus; /**< Boolean indicating whether this file_pane_t has the input focus. */
			text_field_t *field; /**< The text_field_t which is the alternative input method for providing a file name. */
			int column_widths[_T3_WDIGET_FP_MAX_COLUMNS], /**< Width in cells of the various columns. */
				column_positions[_T3_WDIGET_FP_MAX_COLUMNS], /**< Left-most position for each column. */
				columns_visible, /**< The number of columns that are visible currently. */
				scrollbar_range; /**< Visible range for scrollbar setting. */
			signals::connection content_changed_connection; /**< Connection to #file_list's content_changed signal. */
			cleanup_ptr<search_panel_t>::t search_panel;

			implementation_t(void) : scrollbar(false), top_idx(0), current(0), file_list(NULL),
				focus(false), field(NULL), columns_visible(0), scrollbar_range(1)
			{}
		};
		pimpl_ptr<implementation_t>::t impl;

		/** Ensure that the updated value of #current does not put the highlighted entry outside the visible range. */
		void ensure_cursor_on_screen(void);
		/** Draw a single item. */
		void draw_line(int idx, bool selected);
		/** Update the width of a single column, based on the items to draw in it. */
		void update_column_width(int column, int start);
		/** Update the widths of all columns. */
		void update_column_widths(void);
		/** Handle a change of contents of #file_list. */
		void content_changed(void);

		void scrollbar_clicked(scrollbar_t::step_t step);
		void scrollbar_dragged(int start);

		void search(const std::string *text);

	public:
		file_pane_t(void);
		virtual ~file_pane_t(void);
		/** Associate a text_field_t with this file_pane_t.
		    The text_field_t will be updated when the selection in this file_pane_t
		    changes.
		*/
		void set_text_field(text_field_t *_field);
		bool process_key(key_t key) override;
		bool set_size(optint height, optint width) override;
		void update_contents(void) override;
		void set_focus(focus_t _focus) override;

		void set_child_focus(window_component_t *target) override;
		bool is_child(window_component_t *component) override;

		bool process_mouse_event(mouse_event_t event) override;

		/** Set the list to its initial position, i.e. the selected item is the first item. */
		void reset(void);
		/** Set the file_list_t that this file_pane_t displays. */
		void set_file_list(file_list_t *_file_list);
		/** Set the current selected item to the named item. */
		void set_file(const std::string *name);

	T3_WIDGET_SIGNAL(activate, void, const std::string *);
};


class T3_WIDGET_LOCAL file_pane_t::search_panel_t : public popup_t {
	private:
		cleanup_t3_window_ptr shadow_window;
		file_pane_t *parent;
		bool redraw;
		text_line_t text;
	public:
		search_panel_t(file_pane_t *_parent);
		bool process_key(key_t key) override;
		void set_position(optint top, optint left) override;
		bool set_size(optint height, optint width) override;
		void update_contents(void) override;
		void show(void) override;
		bool process_mouse_event(mouse_event_t event) override;
};


}; // namespace

#endif
