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
#include "dialogs/finddialog.h"
#include "log.h"
#include "main.h"

#define FIND_DIALOG_WIDTH 50
#define FIND_DIALOG_HEIGHT 10

find_dialog_t::find_dialog_t(bool _replace) :
	dialog_t(FIND_DIALOG_HEIGHT + 2 * _replace, FIND_DIALOG_WIDTH, (screenLines - FIND_DIALOG_HEIGHT - 2 * _replace) /2,
		(screenColumns - FIND_DIALOG_WIDTH) / 2, DIALOG_DEPTH, _replace ? "Replace" : "Find"),
	state(FindFlags::ICASE | FindFlags::WRAP), replace(_replace)
{
	smart_label_t *find_label,
		*replace_label,
		*whole_word_label,
		*match_case_label,
		*regex_label,
		*wrap_label,
		*transform_backslash_label,
		*reverse_direction_label;
	button_t *find_button, *cancel_button, *in_selection_button, *replace_all_button;

	find_label = new smart_label_t(this, NULL, 1, 2, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "S_earch for;eE", true);
	find_line = new text_field_t(this, find_label, 0, 1, FIND_DIALOG_WIDTH - find_label->get_width() - 5, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	find_line->set_label(find_label);
	find_line->set_callback(text_field_t::ENTER, this, OK);

	if (replace) {
		replace_label = new smart_label_t(this, NULL, 2, 2, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "Re_place with;pP", true);
		replace_line = new text_field_t(this, replace_label, 0, 1, FIND_DIALOG_WIDTH - replace_label->get_width() - 5, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
		replace_line->set_label(replace_label);
		replace_line->set_callback(text_field_t::ENTER, this, OK);
	}

	whole_word_checkbox = new checkbox_t(this, 2 + replace, 2, false);
	whole_word_label = new smart_label_t(this, whole_word_checkbox, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "Match whole word _only;oO");
	whole_word_checkbox->set_label(whole_word_label);
	whole_word_checkbox->set_callback(checkbox_t::TOGGLED, this, FindFlags::WHOLE_WORD + ACTION_LAST);
	whole_word_checkbox->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	whole_word_checkbox->set_callback(checkbox_t::MOVE_DOWN, this, FOCUS_NEXT);
	whole_word_checkbox->set_callback(checkbox_t::ENTER, this, OK);
	match_case_checkbox = new checkbox_t(this, 3 + replace, 2, false);
	match_case_label = new smart_label_t(this, match_case_checkbox, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Match case;mM");
	match_case_checkbox->set_label(match_case_label);
	match_case_checkbox->set_callback(checkbox_t::TOGGLED, this, FindFlags::ICASE + ACTION_LAST);
	match_case_checkbox->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	match_case_checkbox->set_callback(checkbox_t::MOVE_DOWN, this, FOCUS_NEXT);
	match_case_checkbox->set_callback(checkbox_t::ENTER, this, OK);
	regex_checkbox = new checkbox_t(this, 4 + replace, 2, false);
	regex_label = new smart_label_t(this, regex_checkbox, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "Regular e_xpression;xX");
	regex_checkbox->set_label(regex_label);
	regex_checkbox->set_callback(checkbox_t::TOGGLED, this, FindFlags::REGEX + ACTION_LAST);
	regex_checkbox->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	regex_checkbox->set_callback(checkbox_t::MOVE_DOWN, this, FOCUS_NEXT);
	regex_checkbox->set_callback(checkbox_t::ENTER, this, OK);
	wrap_checkbox = new checkbox_t(this, 5 + replace, 2, true);
	wrap_label = new smart_label_t(this, wrap_checkbox, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Wrap;wW");
	wrap_checkbox->set_label(wrap_label);
	wrap_checkbox->set_callback(checkbox_t::TOGGLED, this, FindFlags::WRAP + ACTION_LAST);
	wrap_checkbox->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	wrap_checkbox->set_callback(checkbox_t::MOVE_DOWN, this, FOCUS_NEXT);
	wrap_checkbox->set_callback(checkbox_t::ENTER, this, OK);
	transform_backslash_checkbox = new checkbox_t(this, 6 + replace, 2, false);
	transform_backslash_label = new smart_label_t(this, transform_backslash_checkbox, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Transform backslash expressions;tT");
	transform_backslash_checkbox->set_label(transform_backslash_label);
	transform_backslash_checkbox->set_callback(checkbox_t::TOGGLED, this, FindFlags::TRANSFROM_BACKSLASH + ACTION_LAST);
	transform_backslash_checkbox->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	transform_backslash_checkbox->set_callback(checkbox_t::MOVE_DOWN, this, FOCUS_NEXT);
	transform_backslash_checkbox->set_callback(checkbox_t::ENTER, this, OK);
	reverse_direction_checkbox = new checkbox_t(this, 7 + replace, 2, false);
	reverse_direction_label = new smart_label_t(this, reverse_direction_checkbox, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "Re_verse direction;vV");
	reverse_direction_checkbox->set_label(reverse_direction_label);
	reverse_direction_checkbox->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	reverse_direction_checkbox->set_callback(checkbox_t::TOGGLED, this, FindFlags::BACKWARD + ACTION_LAST);
	reverse_direction_checkbox->set_callback(checkbox_t::MOVE_DOWN, this, FOCUS_NEXT);
	reverse_direction_checkbox->set_callback(checkbox_t::ENTER, this, OK);

	cancel_button = new button_t(this, this, -1, -1, -2, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT), "_Cancel;cC", false);
	cancel_button->set_callback(button_t::ENTER, this, CLOSE);
	cancel_button->set_callback(button_t::MOVE_LEFT, this, FOCUS_PREVIOUS);
	/* Nasty trick: registering a callback twice will call the callback twice. We need to do
	   FOCUS_PREVIOUS twice here to emulate moving up, because the find_button is in the way. */
	cancel_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
	cancel_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
	if (replace) {
		//~ cancel_button->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
		cancel_button->set_callback(checkbox_t::MOVE_UP, this, FOCUS_PREVIOUS);
	}
	find_button = new button_t(this, cancel_button, -1, 0, -2, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT), "_Find;fF", true);
	find_button->set_callback(button_t::ENTER, this, OK);
	find_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
	find_button->set_callback(button_t::MOVE_RIGHT, this, FOCUS_NEXT);
	if (replace) {
		find_button->set_callback(button_t::MOVE_LEFT, this, FOCUS_PREVIOUS);
		find_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
		find_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);

		replace_all_button = new button_t(this, find_button, -1, 0, -2, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT), "_All;aA", false);
		replace_all_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
		replace_all_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
		replace_all_button->set_callback(button_t::MOVE_DOWN, this, FOCUS_NEXT);
		replace_all_button->set_callback(button_t::MOVE_DOWN, this, FOCUS_NEXT);
		replace_all_button->set_callback(button_t::MOVE_DOWN, this, FOCUS_NEXT);
		replace_all_button->set_callback(button_t::MOVE_LEFT, this, FOCUS_PREVIOUS);
		replace_all_button->set_callback(button_t::MOVE_RIGHT, this, FOCUS_NEXT);
		replace_all_button->set_callback(button_t::ENTER, this, REPLACE_ALL);

		in_selection_button = new button_t(this, this, -1, -2, -2, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT), "In _Selection;sS", false);
		in_selection_button->set_callback(button_t::ENTER, this, -1);
		in_selection_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
		in_selection_button->set_callback(button_t::MOVE_DOWN, this, FOCUS_NEXT);
		in_selection_button->set_callback(button_t::MOVE_DOWN, this, FOCUS_NEXT);
		in_selection_button->set_callback(button_t::MOVE_RIGHT, this, FOCUS_NEXT);
		in_selection_button->set_callback(button_t::ENTER, this, IN_SELECTION);
	}


	components.push_back(find_label);
	components.push_back(find_line);
	if (replace) {
		components.push_back(replace_label);
		components.push_back(replace_line);
	}
	components.push_back(whole_word_checkbox);
	components.push_back(whole_word_label);
	components.push_back(match_case_label);
	components.push_back(match_case_checkbox);
	components.push_back(regex_label);
	components.push_back(regex_checkbox);
	components.push_back(wrap_label);
	components.push_back(wrap_checkbox);
	components.push_back(transform_backslash_label);
	components.push_back(transform_backslash_checkbox);
	components.push_back(reverse_direction_label);
	components.push_back(reverse_direction_checkbox);
	if (replace) {
		components.push_back(in_selection_button);
		components.push_back(replace_all_button);
	}
	components.push_back(find_button);
	components.push_back(cancel_button);

	draw_dialog();
}

bool find_dialog_t::resize(optint height, optint width, optint top, optint left) {
	(void) height;
	(void) width;
	(void) top;
	(void) left;
	return dialog_t::resize(None, None, (screenLines - FIND_DIALOG_HEIGHT + replace) /2, (screenColumns - FIND_DIALOG_WIDTH) / 2);
}

void find_dialog_t::callback(int action, const void *data) {
	switch (action) {
		case OK: {
			bool result;
			deactivate_window();
			result = find(find_line->get_text(), state, replace ? replace_line->get_line() : NULL);
			if (replace && result)
				activate_window(WindowID::REPLACE_BUTTONS);
			//FIXME: for replace the button-only window should be shown, in such a way that it doesn't overlap with
			//the found text (but only if the text was found at all!)
			break;
		}
		case FindFlags::BACKWARD + ACTION_LAST:
		case FindFlags::ICASE + ACTION_LAST:
		case FindFlags::REGEX + ACTION_LAST:
		case FindFlags::WRAP + ACTION_LAST:
		case FindFlags::TRANSFROM_BACKSLASH + ACTION_LAST:
		case FindFlags::WHOLE_WORD + ACTION_LAST:
			state ^= (action - ACTION_LAST);
			break;
		case REPLACE:
		case REPLACE_ALL:
		case IN_SELECTION:
			break;
		default:
			dialog_t::callback(action, data);
			break;
	}
}

replace_buttons_dialog_t::replace_buttons_dialog_t(void) :
		dialog_t(3, 60, (screenLines - 3) / 2, (screenColumns - 60) / 2, DIALOG_DEPTH, "Replace")
{
	button_t *find_button, *cancel_button, *replace_all_button, *replace_button;
	int dialog_width;

	replace_all_button = new button_t(this, this, -1, 1, 2, T3_PARENT(T3_ANCHOR_TOPLEFT), "_All;aA", false);
	replace_all_button->set_callback(button_t::MOVE_RIGHT, this, FOCUS_NEXT);
	replace_all_button->set_callback(button_t::ENTER, this, find_dialog_t::REPLACE_ALL);

	replace_button = new button_t(this, replace_all_button, -1, 0, 2, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Replace;rR", false);
	replace_button->set_callback(button_t::MOVE_LEFT, this, FOCUS_PREVIOUS);
	replace_button->set_callback(button_t::MOVE_RIGHT, this, FOCUS_NEXT);
	replace_button->set_callback(button_t::ENTER, this, find_dialog_t::REPLACE);

	find_button = new button_t(this, replace_button, -1, 0, 2, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Find;fF", false);
	find_button->set_callback(button_t::ENTER, this, OK);
	find_button->set_callback(button_t::MOVE_UP, this, FOCUS_PREVIOUS);
	find_button->set_callback(button_t::MOVE_RIGHT, this, FOCUS_NEXT);

	cancel_button = new button_t(this, find_button, -1, 0, 2,  T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Cancel;cC", false);
	cancel_button->set_callback(button_t::ENTER, this, CLOSE);
	cancel_button->set_callback(button_t::MOVE_LEFT, this, FOCUS_PREVIOUS);

	components.push_back(replace_all_button);
	components.push_back(replace_button);
	components.push_back(find_button);
	components.push_back(cancel_button);

	dialog_width = replace_all_button->get_width() + replace_button->get_width() +
		find_button->get_width() + cancel_button->get_width() + 10;
	dialog_t::resize(None, dialog_width, None, (screenColumns - dialog_width) / 2);

	draw_dialog();
}

bool replace_buttons_dialog_t::resize(optint height, optint width, optint top, optint left) {
	int selection_top, selection_bottom;
	int move = 0;

	(void) height;
	(void) width;
	(void) top;
	(void) left;

	get_selection_lines(&selection_top, &selection_bottom);

	if (selection_top <= ((screenLines - 3) / 2) + 3 && selection_bottom >= (screenLines - 3) / 2) {
		move = selection_top - ((screenLines - 3) / 2) - 4;
		if (selection_bottom - (screenLines - 3) / 2 + 1 <= -move)
			move = selection_bottom - (screenLines - 3) / 2 + 1;
	}

	if ((screenLines - 3) / 2 + move < 0 || (screenLines - 3) / 2 + move + 3 >= screenLines)
		move = 0;

	return dialog_t::resize(None, None, (screenLines - 3) / 2 + move, (screenColumns - t3_win_get_width(window)) / 2);
}

void replace_buttons_dialog_t::callback(int action, const void *data) {
	switch (action) {
		case find_dialog_t::REPLACE:
			replace();
			/* FALLTHROUGH */
		case OK: {
			bool result;
			deactivate_window();
			// Any find here will be equivalent to a "find next"
			result = findNext(true);
			if (result) {
				widgets_t::iterator focus_component = current_component;
				activate_window(WindowID::REPLACE_BUTTONS);
				(*current_component)->set_focus(false);
				current_component = focus_component;
				(*current_component)->set_focus(true);
			}
			break;
		}
		case find_dialog_t::REPLACE_ALL:
			break;
		default:
			dialog_t::callback(action, data);
			break;
	}
}

void replace_buttons_dialog_t::set_show(bool show) {
	resize(None, None, None, None);
	dialog_t::set_show(show);
}
