/* Copyright (C) 2011 G.P. Halkes
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
#ifndef T3_WIDGET_WIDGETS_H
#define T3_WIDGET_WIDGETS_H

#include <deque>

#include <t3widget/interfaces.h>

namespace t3_widget {

/** Base class for widgets. */
class T3_WIDGET_API widget_t : public virtual window_component_t, public mouse_target_t {
	private:
		friend class container_t;

		/** Default parent for widgets, making them invisible. */
		static cleanup_t3_window_ptr default_parent;

	protected:
		bool redraw, /**< Widget requires redrawing on next #update_contents call. */
			enabled, /**< Widget is enabled. */
			shown; /**< Widget is shown. */

		/** Constructor which creates a default @c t3_window_t with @p height and @p width. */
		widget_t(int height, int width, bool register_as_mouse_target = true);
		/** Constructor which does not create a default t3_window_t.
		    This constructor should only rarely be necessary. Widgets using this
		    constructor should call either #init_window, or #init_unbacked_window.
		*/
		widget_t(void);

		/** Initialize the #window with a @c t3_window_t with @p height and @p width. */
		void init_window(int height, int width, bool register_as_mouse_target = true);
		/** Initialize the #window with an unbacked @c t3_window_t with @p height and @p width. */
		void init_unbacked_window(int height, int width, bool register_as_mouse_target = false);

	public:
		/** Query whether key is a hotkey for this widget. */
		virtual bool is_hotkey(key_t key);
		/** Query whether this widget accepts focus. */
		virtual bool accepts_focus(void);
		virtual void set_position(optint top, optint left);
		virtual void show(void);
		virtual void hide(void);
		/** Set this widget's anchor.
		    Use @p anchor to position this window. See libt3window's
		    t3_win_set_anchor for details on the @p relation parameter.
		*/
		virtual void set_anchor(window_component_t *anchor, int relation);
		virtual void force_redraw(void);
		/** Set the enabled status of this widget.
		    When a widget is not enabled, it will not accept focus.
		*/
		virtual void set_enabled(bool enable);
		/** Query the enabled status of this widget. */
		virtual bool is_enabled(void);
		/** Query the visibility status of this widget. */
		virtual bool is_shown(void);
		virtual void set_focus(focus_t focus);
		virtual bool process_mouse_event(mouse_event_t event);
};

/** Base class for widgets that take focus. */
class T3_WIDGET_API focus_widget_t {
	/** @fn sigc::connection connect_move_focus_left(const sigc::slot<void> &_slot)
	    Connect a callback to be called on emission of the move_focus_left_signal. */
	/** Signal emitted when the user pressed the left arrow key and focus should move. */
	T3_WIDGET_SIGNAL(move_focus_left, void);
	/** @fn sigc::connection connect_move_focus_right(const sigc::slot<void> &_slot)
	    Connect a callback to be called on emission of the move_focus_right_signal. */
	/** Signal emitted when the user pressed the right arrow key and focus should move. */
	T3_WIDGET_SIGNAL(move_focus_right, void);
	/** @fn sigc::connection connect_move_focus_up(const sigc::slot<void> &_slot)
	    Connect a callback to be called on emission of the move_focus_up_signal. */
	/** Signal emitted when the user pressed the up arrow key and focus should move. */
	T3_WIDGET_SIGNAL(move_focus_up, void);
	/** @fn sigc::connection connect_move_focus_down(const sigc::slot<void> &_slot)
	    Connect a callback to be called on emission of the move_focus_down_signal. */
	/** Signal emitted when the user pressed the down arrow key and focus should move. */
	T3_WIDGET_SIGNAL(move_focus_down, void);
};

typedef std::deque<widget_t *> widgets_t;

}; // namespace
#endif
