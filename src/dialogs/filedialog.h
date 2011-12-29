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
	private:
		struct implementation_t;
		pimpl_ptr<implementation_t>::t impl;

	protected:
		file_dialog_t(int height, int width, const char *_title);

		widget_t *get_anchor_widget(void);
		void insert_extras(widget_t *widget);
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
	private:
		class T3_WIDGET_API filter_text_field_t : public text_field_t {
			public:
				virtual void set_focus(bool _focus);
			T3_WIDGET_SIGNAL(lose_focus, void);
		};

		struct implementation_t;
		pimpl_ptr<implementation_t>::t impl;

		virtual const std::string *get_filter(void);

	public:
		open_file_dialog_t(int height, int width);
		virtual bool set_size(optint height, optint width);
		virtual void reset(void);
};


class T3_WIDGET_API save_as_dialog_t : public file_dialog_t {
	private:
		static std::string empty_filter;

		struct implementation_t;
		pimpl_ptr<implementation_t>::t impl;

	protected:
		virtual const std::string *get_filter(void) { return &empty_filter; }
	public:
		save_as_dialog_t(int height, int width);
		void create_folder(void);
};

}; // namespace
#endif
