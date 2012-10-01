/* Copyright (C) 2012 G.P. Halkes
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
#ifndef T3_WIDGET_EXPANDERGROUP_H
#define T3_WIDGET_EXPANDERGROUP_H

#include <deque>
#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/expander.h>

namespace t3_widget {

/** A widget showing an expander, which allows hiding another widget. */
class T3_WIDGET_API expander_group_t : public widget_t, public container_t, public focus_widget_t {
	private:
		typedef std::deque<expander_t *> expanders_t;

		struct T3_WIDGET_LOCAL implementation_t {
			expanders_t children;
			int current_child, expanded_child, hotkey_activated;
			bool has_focus;
			implementation_t(void) : current_child(-1), expanded_child(-1), hotkey_activated(-1), has_focus(false) {}
		};
		pimpl_ptr<implementation_t>::t impl;

		void focus_next(void);
		void focus_previous(void);
	public:
		/** Create a new expander_t.
		    @param _text The text for the smart_label_t shown next to the expander symbol.
		*/
		expander_group_t(void);
		~expander_group_t(void);

		/** Set the child widget. */
		void add_child(expander_t *child);
		void child_expanded(bool is_expanded, int child_idx);
		void collapse(void);

		virtual bool process_key(key_t key);
		virtual void update_contents(void);
		virtual void set_focus(focus_t _focus);
		virtual bool set_size(optint height, optint width);
		virtual bool accepts_focus(void);
		virtual bool is_hotkey(key_t key);
//		virtual void set_enabled(bool enable);
//		virtual void force_redraw(void);
		virtual void focus_set(window_component_t *target);
		virtual bool is_child(window_component_t *component);

	T3_WIDGET_SIGNAL(expanded, void, bool);
};

}; // namespace
#endif
