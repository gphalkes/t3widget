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
#ifndef T3_WIDGET_FILEDIALOG_H
#define T3_WIDGET_FILEDIALOG_H

#include <t3widget/interfaces.h>
#include <t3widget/dialogs/dialog.h>
#include <t3widget/widgets/filepane.h>
#include <t3widget/widgets/button.h>
#include <t3widget/widgets/checkbox.h>
#include <t3widget/widgets/frame.h>

namespace t3_widget {

class T3_WIDGET_API file_dialog_t : public dialog_t {
	protected:
		file_name_list_t names;
		filtered_file_list_t view;
		std::string current_dir, lang_codeset_filter;

		int name_offset;

		file_pane_t *file_pane;
		frame_t *file_pane_frame;
		text_field_t *file_line;
		button_t *cancel_button, *ok_button;
		checkbox_t *show_hidden_box;
		smart_label_t *show_hidden_label;
		bool option_widget_set;
		sigc::connection cancel_button_up_connection, ok_button_up_connection, ok_button_left_connection;

		file_dialog_t(int height, int width, const char *_title);
		void ok_callback(void);
		void ok_callback(const std::string *file);
		virtual const std::string *get_filter(void) = 0;

	public:
		virtual bool set_size(optint height, optint width);
		void change_dir(const std::string *dir);
		virtual void set_file(const char *file);
		void refresh_view(void);
		void set_options_widget(widget_t *options);
		virtual void reset(void);

	T3_WIDGET_SIGNAL(file_selected, void, const std::string *);
};

class T3_WIDGET_API open_file_dialog_t : public file_dialog_t {
	protected:
		class T3_WIDGET_API filter_text_field_t : public text_field_t {
			public:
				virtual void set_focus(bool _focus);
			T3_WIDGET_SIGNAL(lose_focus, void);
		};

		int filter_offset,
			filter_width;
		filter_text_field_t *filter_line;
		smart_label_t *filter_label;

		virtual const std::string *get_filter(void);

	public:
		open_file_dialog_t(int height, int width);
		virtual void reset(void);
};


class T3_WIDGET_API save_as_dialog_t : public file_dialog_t {
	protected:
		button_t *create_button;
		static std::string empty_filter;

		virtual const std::string *get_filter(void) { return &empty_filter; }
	public:
		save_as_dialog_t(int height, int width);
		void create_folder(void);
};

}; // namespace
#endif
