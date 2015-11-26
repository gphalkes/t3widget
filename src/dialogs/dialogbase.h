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

#ifndef T3_DIALOG_BASE_H
#define T3_DIALOG_BASE_H

#include <list>
#include <t3widget/interfaces.h>
#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/dummywidget.h>

namespace t3_widget {

class dialog_base_t;
typedef std::list<dialog_base_t *> dialog_base_list_t;
class dialog_t;

class T3_WIDGET_API dialog_base_t : public virtual window_component_t, public container_t {
	private:
		friend class dialog_t;

		static dialog_base_list_t dialog_base_list; /**< List of all dialogs in the application. */
		static dummy_widget_t *dummy; /**< Dummy widget to ensure that a dialog is never empty when shown. */

		static void init(bool _init); /**< Function to initialize the dummy widget. */
		static signals::connection init_connected; /**< Dummy value to allow static connection of the @c on_init signal to #init. */

		/** Default constructor, made private to avoid use. */
		dialog_base_t(void);

	protected:
		widgets_t widgets; /**< List of widgets on this dialog. This list should only be filled using #push_back. */
		widgets_t::iterator current_widget; /**< Iterator indicating the widget that has the input focus. */
		bool redraw; /**< Boolean indicating whether redrawing is necessary. */
		cleanup_t3_window_ptr shadow_window; /**< t3_window_t used to draw the shadow under a dialog. */

		/** Create a new dialog with @p height and @p width, and with title @p _title. */
		dialog_base_t(int height, int width, bool has_shadow);
		/** Focus the previous widget, wrapping around if necessary. */
		void focus_next(void);
		/** Focus the next widget, wrapping around if necessary. */
		void focus_previous(void);
		/** Add a widget to this dialog.
		    If a widget is not added through #push_back, it will not be
		    displayed, or receive input. */
		void push_back(widget_t *widget);

		virtual bool is_child(window_component_t *widget);
		virtual void set_child_focus(window_component_t *target);

	public:
		/** Destroy this dialog.
		    Any widgets on the dialog are deleted as well.
		*/
		virtual ~dialog_base_t();
		virtual void set_position(optint top, optint left);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(focus_t focus);
		virtual void show(void);
		virtual void hide(void);
		virtual void force_redraw(void);
		/** Set the position and anchoring for this dialog such that it is centered over a window_component_t. */
		virtual void center_over(window_component_t *center);

		/** Call #force_redraw on all dialogs. */
		static void force_redraw_all(void);

};

} // namespace

#endif // T3_DIALOG_BASE_H
