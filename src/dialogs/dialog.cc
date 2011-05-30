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
#include "colorscheme.h"
#include "main.h"
#include "dialogs/dialog.h"
#include "dialogs/mainwindow.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

dialogs_t dialog_t::active_dialogs;
dialogs_t dialog_t::dialogs;
int dialog_t::dialog_depth;
dummy_widget_t *dialog_t::dummy;
bool dialog_t::init_connected = connect_on_init(sigc::ptr_fun(dialog_t::init));

void dialog_t::init(void) {
	dummy = new dummy_widget_t();
}

dialog_t::dialog_t(int height, int width, const char *_title) : active(false), shadow_window(NULL),
		title(_title), redraw(true)
{
	if ((window = t3_win_new(NULL, height, width, 0, 0, 0)) == NULL)
		throw bad_alloc();
	if ((shadow_window = t3_win_new(NULL, height, width, 1, 1, 1)) == NULL)
		throw bad_alloc();
	t3_win_set_anchor(shadow_window, window, 0);
	t3_win_set_default_attrs(shadow_window, T3_ATTR_REVERSE);
	dialogs.push_back(this);
	t3_win_set_restrict(window, NULL);
}

/** Create a new ::dialog_t.

    This constructor should only be called by ::main_window_base_t.
*/
dialog_t::dialog_t(void) : active(false), shadow_window(NULL), title(NULL), redraw(false) {
	dialogs.push_back(this);
}

dialog_t::~dialog_t() {
	t3_win_del(window);
	t3_win_del(shadow_window);
	for (dialogs_t::iterator iter = dialogs.begin(); iter != dialogs.end(); iter++) {
		if ((*iter) == this) {
			dialogs.erase(iter);
			break;
		}
	}
}

void dialog_t::activate_dialog(void) {
	if (!active_dialogs.empty()) {
		if (this == active_dialogs.back())
			return;

		active_dialogs.back()->set_focus(false);
		if (active) {
			for (dialogs_t::iterator iter = active_dialogs.begin(); iter != active_dialogs.end(); iter++) {
				if (*iter == this) {
					active_dialogs.erase(iter);
					break;
				}
			}
		}
	}

	active = true;
	set_focus(true);
	dialog_depth -= 2;
	t3_win_set_depth(window, dialog_depth);
	if (shadow_window != NULL)
		t3_win_set_depth(shadow_window, dialog_depth + 1);
	active_dialogs.push_back(this);
}

void dialog_t::deactivate_dialog(void) {
	this->active = false;
	if (this == active_dialogs.back()) {
		this->set_focus(false);
		active_dialogs.pop_back();
		active_dialogs.back()->set_focus(true);
		dialog_depth = t3_win_get_depth(active_dialogs.back()->window);
		return;
	}

	for (dialogs_t::iterator iter = active_dialogs.begin(); iter != active_dialogs.end(); iter++) {
		if (*iter == this) {
			active_dialogs.erase(iter);
			break;
		}
	}
}

void dialog_t::draw_dialog(void) {
	int i, x;

	redraw = false;
	t3_win_set_default_attrs(window, attributes.dialog);

	/* Just clear the whole thing and redraw */
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	t3_win_box(window, 0, 0, t3_win_get_height(window), t3_win_get_width(window), 0);
	if (title != NULL) {
		t3_win_set_paint(window, 0, 2);
		t3_win_addstr(window, "[ ", 0);
		t3_win_addstr(window, title, 0);
		t3_win_addstr(window, " ]", 0);
	}

	x = t3_win_get_width(shadow_window) - 1;
	for (i = t3_win_get_height(shadow_window) - 1; i > 0; i--) {
		t3_win_set_paint(shadow_window, i - 1, x);
		t3_win_addch(shadow_window, ' ', 0);
	}
	t3_win_set_paint(shadow_window, t3_win_get_height(shadow_window) - 1, 0);
	t3_win_addchrep(shadow_window, ' ', 0, t3_win_get_width(shadow_window));
}

bool dialog_t::process_key(key_t key) {
	if ((key & EKEY_META) || key == EKEY_F10) {
		for (widgets_t::iterator iter = widgets.begin();
				iter != widgets.end(); iter++) {
			if ((*iter)->is_enabled() && (*iter)->is_hotkey(key & ~EKEY_META)) {
				if ((*iter)->accepts_focus()) {
					(*current_widget)->set_focus(false);
					current_widget = iter;
					(*current_widget)->set_focus(true);
				}
				if ((*iter)->process_key(EKEY_HOTKEY))
					return true;
			}
		}
		return false;
	}

	if ((*current_widget)->process_key(key))
		return true;

	switch (key) {
		case EKEY_ESC:
			close();
			break;
		case '\t':
			focus_next();
			break;
		case EKEY_SHIFT | '\t':
			focus_previous();
			break;
		default:
			return false;
	}
	return true;
}

void dialog_t::set_position(optint top, optint left) {
	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);

	t3_win_move(window, top, left);
}

bool dialog_t::set_size(optint height, optint width) {
	bool result = true;

	redraw = true;
	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_width(window);

	result &= (t3_win_resize(window, height, width) == 0);
	result &= (t3_win_resize(shadow_window, height, width) == 0);
	return result;
}


void dialog_t::update_contents(void) {
	if (redraw)
		draw_dialog();

	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->update_contents();
}

void dialog_t::set_focus(bool focus) {
	(*current_widget)->set_focus(focus);
}

void dialog_t::show(void) {
	for (current_widget = widgets.begin();
		current_widget != widgets.end() && !(*current_widget)->accepts_focus();
		current_widget++)
	{}

	if (current_widget == widgets.end()) {
		widgets.push_front(dummy);
		current_widget = widgets.begin();
	}

	activate_dialog();

	t3_win_show(window);
	if (shadow_window != NULL)
		t3_win_show(shadow_window);
}

void dialog_t::hide(void) {
	deactivate_dialog();
	t3_win_hide(window);
	if (shadow_window != NULL)
		t3_win_hide(shadow_window);
	if (widgets.front() == dummy)
		widgets.pop_front();
}

void dialog_t::close(void) {
	hide();
	closed();
}

void dialog_t::focus_next(void) {
	(*current_widget)->set_focus(false);
	do {
		current_widget++;
		if (current_widget == widgets.end())
			current_widget = widgets.begin();
	} while (!(*current_widget)->accepts_focus());

	(*current_widget)->set_focus(true);
}

void dialog_t::focus_previous(void) {
	(*current_widget)->set_focus(false);

	do {
		if (current_widget == widgets.begin())
			current_widget = widgets.end();

		current_widget--;
	} while (!(*current_widget)->accepts_focus());

	(*current_widget)->set_focus(true);
}

void dialog_t::focus_set(widget_t *target) {
	if (!target->accepts_focus())
		return;

	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
		if (*iter == target) {
			(*current_widget)->set_focus(false);
			current_widget = iter;
			(*current_widget)->set_focus(true);
			return;
		}
	}
}

void dialog_t::push_back(widget_t *widget) {
	if (!set_widget_parent(widget))
		return;
	widgets.push_back(widget);
}

void dialog_t::force_redraw(void) {
	for (widgets_t::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		(*iter)->force_redraw();
}

void dialog_t::center_over(window_component_t *center) {
	t3_win_set_anchor(window, center->get_draw_window(), T3_PARENT(T3_ANCHOR_CENTER) | T3_CHILD(T3_ANCHOR_CENTER));
	t3_win_move(window, 0, 0);
}

void dialog_t::force_redraw_all(void) {
	for (dialogs_t::iterator iter = dialogs.begin(); iter != dialogs.end(); iter++)
		(*iter)->force_redraw();
}

}; // namespace
