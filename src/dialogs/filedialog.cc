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

#include "window/window.h"

#include "dialogs/filedialog.h"
#include "main.h"
#include "util.h"


static key_t nul = 0;
/* FIXME: TODO:
	- i18n/l10n
	- ensure consistent naming (isdir vs is_dir)
	- left-right navigation through buttons etc.
	- path-name cleansing ( /foo/../bar -> /bar, ////usr -> /usr etc.)
*/
#warning FIXME: allow some way to specify that an extra button for options should be present
	//FIXME: perhaps we should also allow a drop down list, which would require different handling
#warning FIXME: complete focus moving
file_dialog_t::file_dialog_t(int height, int width, const char *_title) : dialog_t(height, width, 2, 1, DIALOG_DEPTH, _title), view(NULL) {
	name_label = new smart_label_t(this, 1, 2, "_Name;nN", true);
	name_offset = name_label->get_width() + 2 + 1; // 2 for offset of "Name", 1 for space in ": ["
	file_line = new text_field_t(this, name_label, 0, 1, t3_win_get_width(window) - 2 - name_offset, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	file_line->connect_activate(sigc::mem_fun0(this, &file_dialog_t::ok_callback));
	file_line->set_label(name_label);
	file_line->set_key_filter(&nul, 1, false);

	file_pane = new file_pane_t(window, height - 5, width - 4, 2, 2);
	file_pane->set_file_list(&names);
	file_pane->set_text_field(file_line);
	file_pane->connect_activate(sigc::mem_fun1(this, &file_dialog_t::ok_callback));

	show_hidden_box = new checkbox_t(this, NULL, -2, 2, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT), false);
	show_hidden_box->connect_toggled(sigc::bind(sigc::mem_fun(this, &file_dialog_t::refresh_view), (const string *) NULL));
	show_hidden_box->connect_activate(sigc::mem_fun0(this, &file_dialog_t::ok_callback));

	show_hidden_label = new smart_label_t(this, show_hidden_box, 0, 1, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Show hidden;sS");
	show_hidden_box->set_label(show_hidden_label);

	cancel_button = new button_t(this, this, -1, -1, -2, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT), "_Cancel;cC", false);
	cancel_button->connect_activate(sigc::bind(sigc::mem_fun(this, &file_dialog_t::set_show), false));
	ok_button = new button_t(this, cancel_button, -1, 0, -2, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPRIGHT), "_OK;oO", true);
	ok_button->connect_activate(sigc::mem_fun0(this, &file_dialog_t::ok_callback));
	ok_button->connect_move_focus_left(sigc::mem_fun(this, &file_dialog_t::focus_previous));
	ok_button->connect_move_focus_right(sigc::mem_fun(this, &file_dialog_t::focus_next));

	encoding_button = new button_t(this, this, -1, -2, -2, T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_BOTTOMRIGHT), "_Encoding;eE", false);
	#warning FIXME: this should probably not be here
	//~ encoding_button->set_callback(button_t::ENTER, this, CHOOSE_ENCODING);

	filter_create_button_offset = show_hidden_label->get_width() + 2 + 2 + 4; // 2 for window offset, 2 for offset of "Show hidden", 1 for extra space, 4 for "[ ] " preceding text
	show = false;

	widgets.push_back(name_label);
	widgets.push_back(file_line);
	widgets.push_back(file_pane);
	widgets.push_back(show_hidden_box);
	widgets.push_back(show_hidden_label);
	widgets.push_back(encoding_button);
	widgets.push_back(ok_button);
	widgets.push_back(cancel_button);
}

bool file_dialog_t::resize(optint height, optint width, optint top, optint left) {
	(void) top;
	(void) left;

//FIXME: check return values
	dialog_t::resize(height, width, None, None);

	file_line->resize(None, t3_win_get_width(window) - 3 - name_offset, None, None);

	file_pane->resize(height - 5, width - 4, 2, 2);

	/* Just clear the whole thing and redraw */
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	draw_dialog();
	return true;
}

void file_dialog_t::set_show(bool _show) {
	if (_show == show)
		return;
	show = _show;

	dialog_t::set_show(show);
	if (show)
		file_pane->reset();
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
	refresh_view(&file_string);
}

void file_dialog_t::ok_callback(void) {
	ok_callback(file_line->get_text());
}

void file_dialog_t::ok_callback(const string *file) {
	if ((*file)[0] == '/') {
		#warning FIXME: this does not seem to make much sense!
		change_dir(file);
	} else if (file_name_list_t::is_dir(&current_dir, file->c_str())) {
		change_dir(file);
		file_line->set_text("");
	} else {
		string full_name = current_dir;
		full_name += "/";
		full_name += *file;
		set_show(false);
		file_selected(&full_name);
	}
}

void file_dialog_t::change_dir(const string *dir) {
	file_name_list_t new_names;
	string new_dir, file_string;
	if (dir->compare("..") == 0) {
		size_t idx = current_dir.rfind('/');

		//FIXME: take into account inaccessible directories!
		/* Check whether we can load the dir. If not, show message and don't change
		   state. */

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

	try {
		new_names.load_directory(&new_dir);
	} catch (int error) {
		string message;
#warning FIXME: show error message
/*		printfInto(&message, "Couldn't change to directory '%s': %s", dir->c_str(), strerror(error));
		activate_window(WindowID::ERROR_DIALOG, &message);*/
		return;
	}

	names = new_names;
	current_dir = new_dir;
	file_line->set_autocomplete(&names);
	refresh_view(&file_string);
}

void file_dialog_t::refresh_view(const string *file) {
	if (view != NULL)
		delete view;
	view = new file_name_list_t::file_name_list_view_t(&names, show_hidden_box->get_state(), get_filter());
	file_pane->set_file_list(view);
	file_pane->set_file(view->get_index(file));
}

open_file_dialog_t::open_file_dialog_t(int height, int width) : file_dialog_t(height, width, "Open File") {
	widgets_t::iterator iter;

	filter_label = new smart_label_t(this, show_hidden_label, 0, 2, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "_Filter;fF", true);
	filter_offset = filter_label->get_width() + 1;
	filter_width = 10;
	filter_line = new text_field_t(this, filter_label, 0, 1, filter_width, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	filter_line->set_text("*");
	filter_line->connect_activate(sigc::bind(sigc::mem_fun(this, &open_file_dialog_t::refresh_view), (const string *) NULL));
	filter_line->connect_lose_focus(sigc::bind(sigc::mem_fun(this, &open_file_dialog_t::refresh_view), (const string *) NULL));
	filter_line->set_label(filter_label);
	filter_line->set_key_filter(&nul, 1, false);

	title = "Open File";

	for (iter = widgets.begin(); iter != widgets.end() && *iter != show_hidden_box; iter++) {}
	iter++;
	widgets.insert(iter, filter_label);
	widgets.insert(iter, filter_line);
	draw_dialog();
}

const string *open_file_dialog_t::get_filter(void) {
	return filter_line->get_text();
}

void open_file_dialog_t::set_file(const char *file) {
	filter_line->set_text("*");
	file_dialog_t::set_file(file);
}

string save_as_dialog_t::empty_filter("*");

save_as_dialog_t::save_as_dialog_t(int height, int width) : file_dialog_t(height, width, "Save File As") {
	widgets_t::iterator iter;

	create_button = new button_t(this, show_hidden_label, -1, 0, 2, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT), "Create Folder", false);
	create_button->connect_activate(sigc::mem_fun(this, &save_as_dialog_t::create_folder));
	for (iter = widgets.begin(); iter != widgets.end() && *iter != show_hidden_box; iter++) {}
	iter++;
	widgets.insert(iter, create_button);
	draw_dialog();
}

void save_as_dialog_t::create_folder(void) {
	#warning FIXME: create folder here
}

