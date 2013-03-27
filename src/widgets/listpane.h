/* Copyright (C) 2011-2012 G.P. Halkes
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

#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/scrollbar.h>

namespace t3_widget {

class T3_WIDGET_API list_pane_t : public widget_t, public container_t {
	private:
		class T3_WIDGET_LOCAL indicator_widget_t : public widget_t {
			private:
				bool has_focus;
			public:
				indicator_widget_t(void);
				virtual bool process_key(key_t key);
				virtual void update_contents(void);
				virtual void set_focus(focus_t focus);
				virtual bool set_size(optint height, optint width);
				virtual bool accepts_focus(void);
		};

		struct T3_WIDGET_LOCAL implementation_t {
			size_t top_idx, current;
			cleanup_t3_window_ptr widgets_window;
			widgets_t widgets;
			bool has_focus;
			scrollbar_t scrollbar;
			bool indicator;
			bool single_click_activate;
			cleanup_ptr<indicator_widget_t>::t indicator_widget;

			implementation_t(bool _indicator) : top_idx(0), current(0),
				has_focus(false), scrollbar(true), indicator(_indicator),
				single_click_activate(false)
			{}
		};
		pimpl_ptr<implementation_t>::t impl;


		void ensure_cursor_on_screen(void);
		void scroll(int change);
		void scrollbar_clicked(scrollbar_t::step_t step);

	protected:
		virtual bool set_widget_parent(window_component_t *widget);

	public:
		list_pane_t(bool _indicator);
		virtual ~list_pane_t(void);
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(focus_t focus);
		virtual void set_anchor(window_component_t *anchor, int relation);
		virtual void force_redraw(void);
		virtual void set_child_focus(window_component_t *target);
		virtual bool is_child(window_component_t *widget);
		virtual bool process_mouse_event(mouse_event_t event);
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

		size_t get_current(void) const;
		void set_current(size_t idx);

		void set_single_click_activate(bool sca);

	T3_WIDGET_SIGNAL(activate, void);
	T3_WIDGET_SIGNAL(selection_changed, void);
};

}; // namespace
#endif
