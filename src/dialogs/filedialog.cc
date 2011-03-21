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
#include <string>
#include <cstring>
#include <cerrno>

#include "window/window.h"

#include "dialogs/filedialog.h"
#include "main.h"
#include "util.h"
#include "internal.h"

using namespace std;
namespace t3_widget {

static key_t nul = 0;
/* FIXME: TODO:
	- i18n/l10n
	- path-name cleansing ( /foo/../bar -> /bar, ////usr -> /usr etc.)
*/
file_dialog_t::file_dialog_t(int height, int width, const char *_title) : dialog_t(height, width, _title),
		view(&names), option_widget_set(false)
{
	smart_label_t *name_label;

	name_label = new smart_label_t(this, "_Name;nN", true);
	name_label->set_position(1, 2);
	name_offset = name_label->get_width() + 2 + 1; // 2 for offset of "Name", 1 for space in ": ["
	file_line = new text_field_t(this);
	file_line->set_anchor(name_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	file_line->set_position(0, 1);
	file_line->set_size(None, width - 2 - name_offset);
	file_line->connect_activate(sigc::mem_fun0(this, &file_dialog_t::ok_callback));
	file_line->set_label(name_label);
	file_line->set_key_filter(&nul, 1, false);
	file_line->set_autocomplete(&view);

	file_pane = new file_pane_t(this);
	file_pane->set_size(height - 4, width - 4);
	file_pane->set_position(2, 2);
	file_pane->set_file_list(&names);
	file_pane->set_text_field(file_line);
	file_pane->connect_activate(sigc::mem_fun1(this, &file_dialog_t::ok_callback));
	file_pane->set_file_list(&view);

	show_hidden_box = new checkbox_t(this, false);
	show_hidden_box->set_anchor(file_pane, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	show_hidden_box->set_position(0, 0);
	show_hidden_box->connect_toggled(sigc::mem_fun(this, &file_dialog_t::refresh_view));
	show_hidden_box->connect_activate(sigc::mem_fun0(this, &file_dialog_t::ok_callback));
	show_hidden_box->connect_move_focus_up(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	show_hidden_box->connect_move_focus_right(sigc::mem_fun(this, &file_dialog_t::focus_next));

	show_hidden_label = new smart_label_t(this, "_Show hidden;sS");
	show_hidden_label->set_anchor(show_hidden_box, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	show_hidden_label->set_position(0, 1);
	show_hidden_box->set_label(show_hidden_label);

	cancel_button = new button_t(this, "_Cancel;cC");
	cancel_button->set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT));
	cancel_button->set_position(-1, -2);
	cancel_button->connect_activate(sigc::mem_fun(this, &file_dialog_t::hide));
	cancel_button->connect_move_focus_left(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	cancel_button_up_connection = cancel_button->connect_move_focus_up(
		sigc::bind(sigc::mem_fun(this, &file_dialog_t::move_focus), file_pane));
	ok_button = new button_t(this, "_OK;oO", true);
	ok_button->set_anchor(cancel_button, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	ok_button->set_position(0, -2);
	ok_button->connect_activate(sigc::mem_fun0(this, &file_dialog_t::ok_callback));
	ok_button_left_connection = ok_button->connect_move_focus_left(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	ok_button->connect_move_focus_right(sigc::mem_fun(this, &file_dialog_t::focus_next));
	ok_button_up_connection = ok_button->connect_move_focus_up(
		sigc::bind(sigc::mem_fun(this, &file_dialog_t::move_focus), file_pane));

	widgets.push_back(name_label);
	widgets.push_back(file_line);
	widgets.push_back(file_pane);
	widgets.push_back(show_hidden_box);
	widgets.push_back(show_hidden_label);
	widgets.push_back(ok_button);
	widgets.push_back(cancel_button);
}

void file_dialog_t::set_options_widget(widget_t *options) {
	focus_widget_t *focus_widget;

	if (option_widget_set)
		return;

	/* Make the file pane one line less high. */
	file_pane->set_size(t3_win_get_height(window) - 5, None);

	option_widget_set = true;
	widgets.insert(widgets.end() - 2, options);
	options->set_anchor(file_pane, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	options->set_position(0, 0);

	cancel_button_up_connection.disconnect();
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	cancel_button->connect_move_focus_up(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	ok_button_left_connection.disconnect();
	ok_button_up_connection.disconnect();
	ok_button->connect_move_focus_up(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	show_hidden_box->connect_move_focus_down(sigc::bind(sigc::mem_fun(this, &file_dialog_t::move_focus), ok_button));
	dynamic_cast<focus_widget_t *>(*(widgets.end() - 4))->connect_move_focus_down(
		sigc::bind(sigc::mem_fun(this, &file_dialog_t::move_focus), ok_button));

	if ((focus_widget = dynamic_cast<focus_widget_t *>(options)) != NULL) {
		focus_widget->connect_move_focus_up(sigc::bind(sigc::mem_fun(this, &file_dialog_t::move_focus), file_pane));
		focus_widget->connect_move_focus_left(sigc::mem_fun(this, &file_dialog_t::focus_previous));
		focus_widget->connect_move_focus_down(sigc::mem_fun(this, &file_dialog_t::focus_next));
	}
}

bool file_dialog_t::set_size(optint height, optint width) {
	bool result = true;
	result &= dialog_t::set_size(height, width);

	result &= file_line->set_size(None, t3_win_get_width(window) - 3 - name_offset);
	result &= file_pane->set_size(height - 4 - option_widget_set, width - 4);
	return result;
}

void file_dialog_t::set_file(const char *file) {
	size_t idx;
	string file_string;

	current_dir = file_name_list_t::get_directory(file);

	if (file == NULL)
		file_string.clear();
	else
		file_string = file;

	names.load_directory(&current_dir);
	idx = file_string.rfind('/');
	if (idx != string::npos)
		file_string.erase(0, idx + 1);

	file_line->set_autocomplete(&names);
	file_line->set_text(&file_string);
	refresh_view();
}

void file_dialog_t::reset(void) {
	file_line->set_text("");
	file_pane->reset();
}

void file_dialog_t::ok_callback(void) {
	ok_callback(file_line->get_text());
}

void file_dialog_t::ok_callback(const string *file) {
	if (file_name_list_t::is_dir(&current_dir, file->c_str())) {
		change_dir(file);
		file_line->set_text("");
	} else {
		string full_name = current_dir;
		full_name += "/";
		full_name += *file;
		hide();
		file_selected(&full_name);
	}
}

void file_dialog_t::change_dir(const string *dir) {
	file_name_list_t new_names;
	string new_dir, file_string;
	if (dir->compare("..") == 0) {
		size_t idx = current_dir.rfind('/');

		if (idx == string::npos || idx == current_dir.size() - 1)
			return;

		file_string = current_dir.substr(idx + 1);
		if (idx == 0)
			idx++;
		new_dir = current_dir.substr(0, idx);
	} else if ((*dir)[0] == '/') {
		new_dir = *dir;
	} else {
		new_dir = current_dir;
		new_dir += "/";
		new_dir += *dir;
	}

	/* Check whether we can load the dir. If not, show message and don't change state. */
	try {
		new_names.load_directory(&new_dir);
	} catch (int error) {
		string message = _("Couldn't change to directory '");
		message += dir->c_str();
		message += "': ";
		message += strerror(errno);
		message_dialog.set_message(&message);
		message_dialog.center_over(this);
		message_dialog.show();
		return;
	}

	names = new_names;
	current_dir = new_dir;
	view.set_filter((filtered_file_list_t::filter_type_t) sigc::bind(sigc::ptr_fun(glob_filter),
		get_filter(), show_hidden_box->get_state()));
	file_pane->reset();
}

void file_dialog_t::refresh_view(void) {
	view.set_filter((filtered_file_list_t::filter_type_t) sigc::bind(sigc::ptr_fun(glob_filter),
		get_filter(), show_hidden_box->get_state()));

	file_pane->set_file(file_line->get_text());
}

open_file_dialog_t::open_file_dialog_t(int height, int width) : file_dialog_t(height, width, "Open File") {
	filter_label = new smart_label_t(this, "_Filter;fF", true);
	filter_label->set_anchor(show_hidden_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	filter_label->set_position(0, 2);
	filter_offset = filter_label->get_width() + 1;
	filter_width = 10;
	filter_line = new text_field_t(this);
	filter_line->set_anchor(filter_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	filter_line->set_position(0, 1);
	filter_line->set_size(None, filter_width);
	filter_line->set_text("*");
	filter_line->connect_activate(sigc::mem_fun(this, &open_file_dialog_t::refresh_view));
	filter_line->connect_lose_focus(sigc::mem_fun(this, &open_file_dialog_t::refresh_view));
	filter_line->connect_move_focus_up(sigc::mem_fun(this, &open_file_dialog_t::focus_previous));
	filter_line->connect_move_focus_up(sigc::mem_fun(this, &open_file_dialog_t::focus_previous));

	filter_line->set_label(filter_label);
	filter_line->set_key_filter(&nul, 1, false);

	title = "Open File";

	widgets.insert(widgets.end() - 2, filter_label);
	widgets.insert(widgets.end() - 2, filter_line);
}

const string *open_file_dialog_t::get_filter(void) {
	return filter_line->get_text();
}

void open_file_dialog_t::reset(void) {
	filter_line->set_text("*");
}

string save_as_dialog_t::empty_filter("*");

save_as_dialog_t::save_as_dialog_t(int height, int width) : file_dialog_t(height, width, "Save File As") {
	create_button = new button_t(this, "Create Folder");
	create_button->set_anchor(show_hidden_label, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	create_button->set_position(0, 2);
	create_button->connect_activate(sigc::mem_fun(this, &save_as_dialog_t::create_folder));
	widgets.insert(widgets.end() - 2, create_button);
}

void save_as_dialog_t::create_folder(void) {
	//FIXME: create folder here
}

}; // namespace
