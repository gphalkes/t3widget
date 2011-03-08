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
#ifndef T3_WIDGET_FILEDIALOG_H
#define T3_WIDGET_FILEDIALOG_H

#include "interfaces.h"
#include "dialogs/dialog.h"
#include "widgets/filepane.h"
#include "widgets/button.h"
#include "widgets/checkbox.h"

namespace t3_widget {

class file_dialog_t : public dialog_t {
	protected:
		file_name_list_t names;
		file_name_list_t::file_name_list_view_t *view;
		std::string current_dir;

		int name_offset,
			filter_create_button_offset;

		file_pane_t *file_pane;
		text_field_t *file_line;
		//FIXME: many of these are not necessary anymore after the constructor, so they should be moved there.
		button_t *ok_button, *cancel_button, *encoding_button;
		checkbox_t *show_hidden_box;
		smart_label_t *name_label, *show_hidden_label;

		file_dialog_t(int height, int width, const char *_title);
		void ok_callback(void);
		void ok_callback(const std::string *file);
		virtual const std::string *get_filter(void) = 0;

	public:
		virtual bool set_size(optint height, optint width);
		virtual void show(void);
		void change_dir(const std::string *dir);
		virtual void set_file(const char *file);
		void refresh_view(const std::string *file);

	SIGNAL(file_selected, void, std::string *);
};

class open_file_dialog_t : public file_dialog_t {
	private:
		int filter_offset,
			filter_width;
		text_field_t *filter_line;
		smart_label_t *filter_label;

		virtual const std::string *get_filter(void);

	public:
		open_file_dialog_t(int height, int width);
		virtual void set_file(const char *file);
};


class save_as_dialog_t : public file_dialog_t {
	private:
		button_t *create_button;
		static std::string empty_filter;

		virtual const std::string *get_filter(void) { return &empty_filter; }
	public:
		save_as_dialog_t(int height, int width);
		void create_folder(void);
};

}; // namespace
#endif
