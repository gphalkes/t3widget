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
#ifndef LISTPANE_H
#define LISTPANE_H

#include "window/window.h"
#include "widgets/widgets.h"

class list_pane_t : public widget_t {
	private:
		int height, width, top, left;
		size_t top_idx, current;
		container_t *parent;
		t3_window_t *clip_window;
		widgets_t widgets;
		bool focus;
		scrollbar_t *scrollbar;
		bool indicator;

		/* Wrapper around a t3_window_t * to allow passing of a different window
		   to the scrollbar_t. All widgets except the scrollbar use an unbacked
		   window of which only part is actually shown through clipping. */
		class clipwindow_component_t : public window_component_t {
			private:
				t3_window_t *window;
			public:
				clipwindow_component_t(t3_window_t *_clipWindow) : window(_clipWindow) {}
				virtual bool process_key(key_t key) { (void) key; return false; }
				virtual void set_position(optint top, optint left) { (void) top; (void) left; }
				virtual bool set_size(optint height, optint width) { (void) height; (void) width; return true; }
				virtual void update_contents(void) {}
				virtual void set_focus(bool _focus) { (void) _focus; }
				virtual void show(void) {}
				virtual void hide(void) {}
				virtual t3_window_t *get_draw_window(void) { return window; }
		};

		void ensure_cursor_on_screen(void);
	public:
		list_pane_t(container_t *_parent, bool _indicator);
		virtual ~list_pane_t(void);
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		virtual bool accepts_enter(void);
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

		int get_current(void);
		void set_current(int idx);

	SIGNAL(activate, void);
	SIGNAL(selection_changed, void);
};

#endif
