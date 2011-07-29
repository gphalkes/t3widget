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
#ifndef T3_WIDGET_DIALOGS_H
#define T3_WIDGET_DIALOGS_H

#include <list>
#include <t3widget/interfaces.h>
#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/dummywidget.h>

namespace t3_widget {

class dialog_t;
typedef std::list<dialog_t *> dialogs_t;
class complex_error_t;

/** Base class for dialogs. */
class T3_WIDGET_API dialog_t : public virtual window_component_t, public container_t {
	private:
		friend void iterate(void);
		// main_window_base_t should be allowed to call dialog_t(), but no others should
		friend class main_window_base_t;

		static dialogs_t active_dialogs; /**< Dialog stack. */
		static dialogs_t dialogs; /**< List of all dialogs in the application. */
		static int dialog_depth; /**< Depth of the top most dialog in the window stack. */
		static dummy_widget_t *dummy; /**< Dummy widget to ensure that a dialog is never empty when shown. */

		static void init(void); /**< Function to initialize the dummy widget. */
		static sigc::connection init_connected; /**< Dummy value to allow static connection of the @c on_init signal to #init. */

		t3_window_t *shadow_window; /**< t3_window_t used to draw the shadow under a dialog. */

		void activate_dialog(void); /**< Move this dialog up to the top of the dialog and window stack. Called from #show. */
		void deactivate_dialog(void); /**< Remove this dialog from the dialog stack. Called from #hide. */

		bool active; /**< Boolean indicating whether this dialog is currently being shown on screen. */

		/** Default constructor, made private to avoid use. */
		dialog_t(void);

	protected:
		const char *title; /**< The title of this dialog. */
		widgets_t widgets; /**< List of widgets on this dialog. This list should only be filled using #push_back. */
		widgets_t::iterator current_widget; /**< Iterator indicating the widget that has the input focus. */
		bool redraw; /**< Boolean indicating whether redrawing is necessary. */

		/** Create a new dialog with @p height and @p width, and with title @p _title. */
		dialog_t(int height, int width, const char *_title);
		/** Focus the previous widget, wrapping around if necessary. */
		void focus_next(void);
		/** Focus the next widget, wrapping around if necessary. */
		void focus_previous(void);
		/** Set the focus to a specific widget. */
		void focus_set(widget_t *target);
		/** Add a widget to this dialog.
		    If a widget is not added through #push_back, it will not be
		    displayed, or receive input. */
		void push_back(widget_t *widget);
		/** Close the dialog.
		    This function should be called when the dialog is closed by some
		    event originating from this dialog. */
		virtual void close(void);

	public:
		/** Destroy this dialog.
		    Any widgets on the dialog are deleted as well.
		*/
		virtual ~dialog_t();
		virtual bool process_key(key_t key);
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool focus);
		virtual void show(void);
		virtual void hide(void);
		virtual void force_redraw(void);
		/** Set the position and anchoring for this dialog such that it is centered over a window_component_t. */
		virtual void center_over(window_component_t *center);

		/** Call #force_redraw on all dialogs. */
		static void force_redraw_all(void);

	/** @fn sigc::connection connect_closed(const sigc::slot<void> &_slot)
	    Connect a callback to the #closed signal.
	*/
	/** Signal emitted when the dialog is closed by calling #close. */
	T3_WIDGET_SIGNAL(closed, void);
};

}; // namespace
#endif
