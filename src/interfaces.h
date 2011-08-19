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
#ifndef T3_WIDGET_INTERFACES_H
#define T3_WIDGET_INTERFACES_H

#include <list>
#include <t3window/window.h>

#include <t3widget/key.h>
#include <t3widget/util.h>

namespace t3_widget {

/** Abstract base class for all items displayed on screen. */
class T3_WIDGET_API window_component_t {
	protected:
		/** The t3_window_t used for presenting this item on screen (see libt3window). */
		auto_t3_window_t window;
	public:
		/** Base constructor. */
		window_component_t(void);
		/** Base destructor. */
		/* Virtual destructor is required for proper functioning of the delete
		   operator in multiple-inheritance situations. */
		virtual ~window_component_t(void);
		/** Retrieve the t3_window_t for this window_component_t.
		    The returned pointer should be used only for setting anchor
		    positions of other window_component_t's and similar operations.
		*/
		virtual t3_window_t *get_base_window(void);
		/** Handle a key press by the user.
		    @return A boolean indicating whether this window_component_t handled the
		        key press.
		*/
		virtual bool process_key(key_t key) = 0;
		/** Move the window_component_t to a specified position.
		    Note that the position is relative to the anchor point. */
		virtual void set_position(optint top, optint left) = 0;
		/** Set the size of this window_component_t.
		    @param height The new height of the window_component_t in cells, or #None if it should remain unchanged.
		    @param width The new width of the window_component_t in cells, or #None if it should remain unchanged.
			@return A boolean indicating whether all allocations required for the resize
		        operation succeeded.
		*/
		virtual bool set_size(optint height, optint width) = 0;
		/** Update the contents of the window. */
		virtual void update_contents(void) = 0;
		/** Set whether this window_component_t has the input focus.
		    Note that this merely notifies the window_component_t that it should change it's
		    appearance to indicate the user that this window_component_t has the input focus.
		    Which window_component_t actually receives the input events is handled outside of
		    the window_component_t.
		*/
		virtual void set_focus(bool focus) = 0;
		/** Display the window_component_t. */
		virtual void show(void) = 0;
		/** Hide the window_component_t. */
		virtual void hide(void) = 0;
		/** Request that this window_component_t be completely redrawn. */
		virtual void force_redraw(void) = 0;
};

class widget_t;
/** Base class for window_component_t's that are the parents of other window_component_t's */
class T3_WIDGET_API container_t : private virtual window_component_t {
	protected:
		/** Make @p widget a child window of this container_t, by setting its parent window. */
		virtual bool set_widget_parent(widget_t *widget);
		/** Unset the parent window for a @p widget. */
		virtual void unset_widget_parent(widget_t *widget);
};

/** Base class for components which need to center dialogs.

    This base class is specifically made for widgets like edit_window_t which
    need to show dialogs. In some cases it is better to center those dialogs
    over the widget itself, but in other cases it is more intuitive to center
    those dialogs over the containing window_component_t of the widget. The
	latter may be the case when the widget is itself part of a dialog. To
    allow both cases, this interface defines a function to set the
    window_component_t used for centering.
*/
class T3_WIDGET_API center_component_t : private virtual window_component_t {
	protected:
		/** The window_component_t to center over. */
		window_component_t *center_window;
	public:
		/** Create a new center_component_t.
		    The #center_window member will be set to @c this.
		*/
		center_component_t(void);
		/** Set the window_component_t to center over. */
		virtual void set_center_window(window_component_t *_center_window);
};

/** Base class for widgets that need handle user text and draw differently based on the t3_win_can_draw function.

    The terminal capability detection code in libt3window may indicate that the
    terminal is or is not capable of drawing some characters that were believed
    to be (un)drawable based on the setting of @c LANG alone. This means that any
    attributes based on the t3_win_can_draw function need to be recalculated.
    Any widgets implementing this interface will be automatically signalled when
    the terminal capability detection is complete.
*/
class T3_WIDGET_API bad_draw_recheck_t {
	private:
		/** List of widgets to signal on completion of the terminal capability detection. */
		static std::list<bad_draw_recheck_t *> to_signal;
		/** sigc::connection used to initialize the connection to the @c terminal_settings_changed signal. */
		static sigc::connection initialized;
		/** Callback function called on the @c terminal_settings_changed signal. */
		static void bad_draw_recheck_all(void);

	public:
		/** Base constructor.
		    Automatically adds this object to the list of objects to signal.
		*/
		bad_draw_recheck_t(void);
		/** Base destructor. */
		virtual ~bad_draw_recheck_t(void);

		/** Function called on reception of the @c terminal_settings_changed signal. */
		virtual void bad_draw_recheck(void) = 0;
};

}; // namespace
#endif
