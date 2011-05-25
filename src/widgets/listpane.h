/* Copyright (C) 2010 G.P. Halkes
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

#include "widgets/widget.h"
#include "widgets/scrollbar.h"

namespace t3_widget {

class list_pane_t : public widget_t, public container_t {
	private:
		int height;
		size_t top_idx, current;
		t3_window_t *widgets_window;
		widgets_t widgets;
		bool has_focus;
		scrollbar_t scrollbar;
		bool indicator;

		class indicator_widget_t : public widget_t {
			private:
				bool has_focus;
			public:
				indicator_widget_t(void);
				virtual bool process_key(key_t key);
				virtual void update_contents(void);
				virtual void set_focus(bool focus);
				virtual bool set_size(optint height, optint width);
				virtual bool accepts_focus(void);
		};

		indicator_widget_t *indicator_widget;

		void ensure_cursor_on_screen(void);

	protected:
		virtual bool set_widget_parent(widget_t *widget);

	public:
		list_pane_t(bool _indicator);
		virtual ~list_pane_t(void);
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void set_anchor(window_component_t *anchor, int relation);
		virtual void force_redraw(void);
		void reset(void);
		void update_positions(void);

		void push_back(widget_t *widget);
		void push_front(widget_t *widget);
		void pop_back(void);
		void pop_front(void);
		widget_t *back(void);
		widget_t *operator[](int idx);
		size_t size(void);
		bool empty(void);

		typedef size_t iterator;

		iterator erase(iterator position);
		iterator begin(void);
		iterator end(void);

		size_t get_current(void);
		void set_current(size_t idx);

	T3_WIDGET_SIGNAL(activate, void);
	T3_WIDGET_SIGNAL(selection_changed, void);
};

}; // namespace
#endif
