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
#include "colorscheme.h"
#include "main.h"
#include "dialogs/dialog.h"
#include "dialogs/mainwindow.h"
#include "internal.h"

namespace t3_widget {

dialogs_t dialog_t::active_dialogs;
popup_t *dialog_t::active_popup;
int dialog_t::dialog_depth;

dialog_t::dialog_t(int height, int width, const char *_title) : dialog_base_t(height, width, true), active(false), title(_title)
{}

/** Create a new ::dialog_t.

    This constructor should only be called by ::main_window_base_t.
*/
dialog_t::dialog_t() : active(false), title(nullptr) {}

void dialog_t::activate_dialog() {
	if (!active_dialogs.empty()) {
		if (this == active_dialogs.back())
			return;

		active_dialogs.back()->set_focus(window_component_t::FOCUS_OUT);
		if (active) {
			remove_element(active_dialogs, this);
		}
	}

	active = true;
	set_focus(window_component_t::FOCUS_SET);
	dialog_depth -= 2;
	t3_win_set_depth(window, dialog_depth);
	if (shadow_window != nullptr)
		t3_win_set_depth(shadow_window, dialog_depth + 1);
	active_dialogs.push_back(this);
	if (active_popup != nullptr)
		active_popup->hide();
}

void dialog_t::deactivate_dialog() {
	this->active = false;

	if (active_popup != nullptr)
		active_popup->hide();

	if (this == active_dialogs.back()) {
		this->set_focus(window_component_t::FOCUS_OUT);
		active_dialogs.pop_back();
		active_dialogs.back()->set_focus(window_component_t::FOCUS_REVERT);
		dialog_depth = t3_win_get_depth(active_dialogs.back()->window);
		return;
	}

	remove_element(active_dialogs, this);
}

bool dialog_t::process_key(key_t key) {
	if (active_popup != nullptr) {
		if (active_popup->process_key(key))
			return true;
	}

	if ((key & EKEY_META) || key == EKEY_F10) {
		for (widgets_t::iterator iter = widgets.begin();
				iter != widgets.end(); iter++)
		{
			widget_container_t *widget_container;
			widget_t *hotkey_target;

			if (!(*iter)->is_enabled() || !(*iter)->is_shown())
				continue;

			if ((*iter)->is_hotkey(key & ~EKEY_META)) {
				if ((*iter)->accepts_focus()) {
					(*current_widget)->set_focus(window_component_t::FOCUS_OUT);
					current_widget = iter;
					(*current_widget)->set_focus(window_component_t::FOCUS_SET);
				}
				if ((*iter)->process_key(EKEY_HOTKEY))
					return true;
			} else if ((widget_container = dynamic_cast<widget_container_t *>(*iter)) != nullptr &&
					(hotkey_target = widget_container->is_child_hotkey(key)) != nullptr)
			{
				if (hotkey_target->accepts_focus()) {
					(*current_widget)->set_focus(window_component_t::FOCUS_OUT);
					current_widget = iter;
					widget_container->set_child_focus(hotkey_target);
				}
				if ((*iter)->process_key(EKEY_HOTKEY))
					return true;
			}
		}
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

void dialog_t::update_contents() {
	bool redraw_title = redraw;

	dialog_base_t::update_contents();

	if (redraw_title && title != nullptr) {
		t3_win_set_paint(window, 0, 3);
		t3_win_addstr(window, " ", 0);
		t3_win_addstr(window, title, 0);
		t3_win_addstr(window, " ", 0);
	}
}

void dialog_t::show() {
	dialog_base_t::show();
	activate_dialog();
}

void dialog_t::hide() {
	deactivate_dialog();
	dialog_base_t::hide();
}

void dialog_t::close() {
	hide();
	closed();
}

bool dialog_t::is_child(window_component_t *widget) {
	if (active_popup != nullptr && active_popup->is_child(widget))
		return true;
	return dialog_base_t::is_child(widget);
}

void dialog_t::set_child_focus(window_component_t *target) {
	if (active_popup != nullptr && active_popup->is_child(target)) {
		active_popup->set_child_focus(target);
		return;
	}
	dialog_base_t::set_child_focus(target);
}

void dialog_t::set_active_popup(popup_t *popup) {
	if (popup == active_popup)
		return;
	popup_t *prev_active = active_popup;
	active_popup = popup;
	if (prev_active != nullptr)
		prev_active->dialog_base_t::hide();
}

void dialog_t::update_dialogs() {
	for (dialog_t *active_dialog : dialog_t::active_dialogs)
		active_dialog->update_contents();
	if (active_popup)
		active_popup->update_contents();
}

}; // namespace
