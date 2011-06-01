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
#include "findcontext.h"
#include "log.h"
#include "main.h"
#include "internal.h"

using namespace std;

namespace t3_widget {

#define FIND_DIALOG_WIDTH 50
#define FIND_DIALOG_HEIGHT 10

#warning FIXME: keep (limited) history

find_dialog_t::find_dialog_t(int _state) : dialog_t(FIND_DIALOG_HEIGHT, FIND_DIALOG_WIDTH, "Find")
{
	smart_label_t *find_label,
		*whole_word_label,
		*match_case_label,
		*regex_label,
		*wrap_label,
		*transform_backslash_label,
		*reverse_direction_label;
	button_t *find_button, *cancel_button;

	find_label = new smart_label_t("S_earch for", true);
	find_label->set_position(1, 2);
	find_line = new text_field_t();
	find_line->set_anchor(find_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	find_line->set_size(None, FIND_DIALOG_WIDTH - find_label->get_width() - 5);
	find_line->set_position(0, 1);
	find_line->set_label(find_label);
	find_line->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));

	replace_label = new smart_label_t("Re_place with", true);
	replace_label->set_position(2, 2);
	replace_line = new text_field_t();
	replace_line->set_anchor(replace_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	replace_line->set_size(None, FIND_DIALOG_WIDTH - replace_label->get_width() - 5);
	replace_line->set_position(0, 1);
	replace_line->set_label(replace_label);
	replace_line->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	replace_label->hide();
	replace_line->hide();

	whole_word_checkbox = new checkbox_t();
	whole_word_checkbox->set_position(2, 2);
	whole_word_label = new smart_label_t("Match whole word _only");
	whole_word_label->set_anchor(whole_word_checkbox, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	whole_word_label->set_position(0, 1);
	whole_word_checkbox->set_label(whole_word_label);
	whole_word_checkbox->connect_toggled(sigc::mem_fun(this, &find_dialog_t::whole_word_toggled));
	whole_word_checkbox->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	whole_word_checkbox->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	whole_word_checkbox->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	match_case_checkbox = new checkbox_t();
	match_case_checkbox->set_anchor(whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	match_case_checkbox->set_position(0, 0);
	match_case_label = new smart_label_t("_Match case");
	match_case_label->set_anchor(match_case_checkbox, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	match_case_label->set_position(0, 1);
	match_case_checkbox->set_label(match_case_label);
	match_case_checkbox->connect_toggled(sigc::mem_fun(this, &find_dialog_t::icase_toggled));
	match_case_checkbox->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	match_case_checkbox->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	match_case_checkbox->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	regex_checkbox = new checkbox_t();
	regex_checkbox->set_anchor(whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	regex_checkbox->set_position(1, 0);
	regex_label = new smart_label_t("Regular e_xpression");
	regex_label->set_anchor(regex_checkbox, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	regex_label->set_position(0, 1);
	regex_checkbox->set_label(regex_label);
	regex_checkbox->connect_toggled(sigc::mem_fun(this, &find_dialog_t::regex_toggled));
	regex_checkbox->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	regex_checkbox->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	regex_checkbox->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	wrap_checkbox = new checkbox_t();
	wrap_checkbox->set_anchor(whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	wrap_checkbox->set_position(2, 0);
	wrap_label = new smart_label_t("_Wrap");
	wrap_label->set_anchor(wrap_checkbox, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	wrap_label->set_position(0, 1);
	wrap_checkbox->set_label(wrap_label);
	wrap_checkbox->connect_toggled(sigc::mem_fun(this, &find_dialog_t::wrap_toggled));
	wrap_checkbox->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	wrap_checkbox->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	wrap_checkbox->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	transform_backslash_checkbox = new checkbox_t();
	transform_backslash_checkbox->set_anchor(whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	transform_backslash_checkbox->set_position(3, 0);
	transform_backslash_label = new smart_label_t("_Transform backslash expressions");
	transform_backslash_label->set_anchor(transform_backslash_checkbox, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	transform_backslash_label->set_position(0, 1);
	transform_backslash_checkbox->set_label(transform_backslash_label);
	transform_backslash_checkbox->connect_toggled(sigc::mem_fun(this, &find_dialog_t::transform_backslash_toggled));
	transform_backslash_checkbox->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	transform_backslash_checkbox->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	transform_backslash_checkbox->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	reverse_direction_checkbox = new checkbox_t();
	reverse_direction_checkbox->set_anchor(whole_word_checkbox, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	reverse_direction_checkbox->set_position(4, 0);
	reverse_direction_label = new smart_label_t("Re_verse direction");
	reverse_direction_label->set_anchor(reverse_direction_checkbox, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	reverse_direction_label->set_position(0, 1);
	reverse_direction_checkbox->set_label(reverse_direction_label);
	reverse_direction_checkbox->connect_toggled(sigc::mem_fun(this, &find_dialog_t::backward_toggled));
	reverse_direction_checkbox->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	reverse_direction_checkbox->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	reverse_direction_checkbox->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));

	cancel_button = new button_t("_Cancel");
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);
	cancel_button->connect_activate(sigc::mem_fun(this, &find_dialog_t::close));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	find_button = new button_t("_Find", true);
	find_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	find_button->set_position(0, -2);
	find_button->connect_activate(sigc::mem_fun0(this, &find_dialog_t::find_activated));
	find_button->connect_move_focus_right(sigc::mem_fun(this, &find_dialog_t::focus_next));
	find_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	find_button_up_connection = find_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	find_button_up_connection.block();

	replace_all_button = new button_t("_All");
	replace_all_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	replace_all_button->set_position(-2, -2);
	replace_all_button->connect_activate(sigc::bind(sigc::mem_fun1(this, &find_dialog_t::find_activated), find_action_t::REPLACE_ALL));
	replace_all_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	replace_all_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	replace_all_button->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	replace_all_button->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	replace_all_button->connect_move_focus_left(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	replace_all_button->hide();

	in_selection_button = new button_t("In _Selection");
	in_selection_button->set_anchor(replace_all_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	in_selection_button->set_position(0, -1);
	in_selection_button->connect_activate(sigc::bind(sigc::mem_fun1(this, &find_dialog_t::find_activated), find_action_t::REPLACE_IN_SELECTION));
	in_selection_button->connect_move_focus_up(sigc::mem_fun(this, &find_dialog_t::focus_previous));
	in_selection_button->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	in_selection_button->connect_move_focus_down(sigc::mem_fun(this, &find_dialog_t::focus_next));
	in_selection_button->connect_move_focus_right(sigc::mem_fun(this, &find_dialog_t::focus_next));
	in_selection_button->hide();

	push_back(find_label);
	push_back(find_line);
	push_back(replace_label);
	push_back(replace_line);
	push_back(whole_word_checkbox);
	push_back(whole_word_label);
	push_back(match_case_label);
	push_back(match_case_checkbox);
	push_back(regex_label);
	push_back(regex_checkbox);
	push_back(wrap_label);
	push_back(wrap_checkbox);
	push_back(transform_backslash_label);
	push_back(transform_backslash_checkbox);
	push_back(reverse_direction_label);
	push_back(reverse_direction_checkbox);
	push_back(in_selection_button);
	push_back(replace_all_button);
	push_back(find_button);
	push_back(cancel_button);

	set_state(_state);
}

bool find_dialog_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

void find_dialog_t::set_text(const string *str) {
	find_line->set_text(str);
}

#define TOGGLED_CALLBACK(name, flag_name) void find_dialog_t::name##_toggled(void) { state ^= find_flags_t::flag_name; }
TOGGLED_CALLBACK(backward, BACKWARD)
TOGGLED_CALLBACK(icase, ICASE)
TOGGLED_CALLBACK(wrap, WRAP)
TOGGLED_CALLBACK(transform_backslash, TRANSFROM_BACKSLASH)
TOGGLED_CALLBACK(whole_word, WHOLE_WORD)
#undef TOGGLED_CALLBACK

void find_dialog_t::regex_toggled(void) {
	state ^= find_flags_t::REGEX;
	transform_backslash_checkbox->set_enabled(!(state & find_flags_t::REGEX));
}

void find_dialog_t::find_activated(void) {
	find_activated(find_action_t::FIND);
}

void find_dialog_t::find_activated(find_action_t action) {
	try {
		finder_t context(find_line->get_text(), state, replace_line->is_shown() ? replace_line->get_text() : NULL);
		hide();
		activate(action, &context);
	} catch (const char *message) {
		message_dialog->set_message(message);
		message_dialog->center_over(this);
		message_dialog->show();
	}
}

void find_dialog_t::set_replace(bool replace) {
	if (replace == replace_line->is_shown())
		return;

	if (replace) {
		title = _("Replace");
		dialog_t::set_size(FIND_DIALOG_HEIGHT + 2, None); // Also forces redraw
		replace_label->show();
		replace_line->show();
		whole_word_checkbox->set_position(3, 2);
		replace_all_button->show();
		in_selection_button->show();
		find_button_up_connection.unblock();
	} else {
		title = _("Find");
		dialog_t::set_size(FIND_DIALOG_HEIGHT, None); // Also forces redraw
		replace_label->hide();
		replace_line->hide();
		whole_word_checkbox->set_position(2, 2);
		replace_all_button->hide();
		in_selection_button->hide();
		find_button_up_connection.block();
	}
}

void find_dialog_t::set_state(int _state) {
	state = _state;
	whole_word_checkbox->set_state(state & find_flags_t::WHOLE_WORD);
	match_case_checkbox->set_state(state & find_flags_t::ICASE);
	regex_checkbox->set_state(state & find_flags_t::REGEX);
	wrap_checkbox->set_state(state & find_flags_t::WRAP);
	transform_backslash_checkbox->set_state(state & find_flags_t::TRANSFROM_BACKSLASH);
	reverse_direction_checkbox->set_state(state & find_flags_t::BACKWARD);
}

//============= replace_buttons_dialog_t ===============

replace_buttons_dialog_t::replace_buttons_dialog_t(void) : dialog_t(3, 60, "Replace") {
	button_t *cancel_button, *replace_all_button;
	int dialog_width;

	replace_all_button = new button_t("_All");
	replace_all_button->set_position(1, 2);
	replace_all_button->connect_activate(sigc::mem_fun(this, &find_dialog_t::hide));
	replace_all_button->connect_activate(sigc::bind(activate.make_slot(), find_action_t::REPLACE_ALL));
	replace_all_button->connect_move_focus_right(sigc::mem_fun(this, &replace_buttons_dialog_t::focus_next));

	replace_button = new button_t("_Replace");
	replace_button->set_anchor(replace_all_button, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	replace_button->set_position(0, 2);
	replace_button->connect_activate(sigc::mem_fun(this, &find_dialog_t::hide));
	replace_button->connect_activate(sigc::bind(activate.make_slot(), find_action_t::REPLACE));
	replace_button->connect_move_focus_left(sigc::mem_fun(this, &replace_buttons_dialog_t::focus_previous));
	replace_button->connect_move_focus_right(sigc::mem_fun(this, &replace_buttons_dialog_t::focus_next));

	find_button = new button_t("_Find");
	find_button->set_anchor(replace_button, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	find_button->set_position(0, 2);
	find_button->connect_activate(sigc::mem_fun(this, &find_dialog_t::hide));
	find_button->connect_activate(sigc::bind(activate.make_slot(), find_action_t::SKIP));
	find_button->connect_move_focus_left(sigc::mem_fun(this, &replace_buttons_dialog_t::focus_previous));
	find_button->connect_move_focus_right(sigc::mem_fun(this, &replace_buttons_dialog_t::focus_next));

	cancel_button = new button_t("_Cancel");
	cancel_button->set_anchor(find_button, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	cancel_button->set_position(0, 2);
	cancel_button->connect_activate(sigc::mem_fun(this, &find_dialog_t::close));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &replace_buttons_dialog_t::focus_previous));

	push_back(replace_all_button);
	push_back(replace_button);
	push_back(find_button);
	push_back(cancel_button);

	dialog_width = replace_all_button->get_width() + replace_button->get_width() +
		find_button->get_width() + cancel_button->get_width() + 10;
	dialog_t::set_size(None, dialog_width);
}

void replace_buttons_dialog_t::reshow(find_action_t button) {
	show();
	switch (button) {
		case find_action_t::REPLACE:
			focus_set(replace_button);
			break;
		case find_action_t::SKIP:
			focus_set(find_button);
			break;
		default:
			break;
	}
}

}; // namespace
