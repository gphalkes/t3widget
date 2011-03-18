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
#ifndef T3_WIDGET_TEXTFIELD_H
#define T3_WIDGET_TEXTFIELD_H

#include <string>

#include "widgets/widget.h"
#include "textline.h"
#include "widgets/contentlist.h"
#include "widgets/smartlabel.h"

namespace t3_widget {

class text_field_t : public complex_widget_t, public focus_widget_t {
	protected:
		int width,
			pos,
			screen_pos,
			leftcol,
			selection_start_pos,
			selection_end_pos;

		selection_mode_t selection_mode;
		bool focus, in_drop_down_list, dont_select_on_focus, edited;

		text_line_t *line;
		const key_t *filter_keys;
		size_t filter_keys_size;
		bool filter_keys_accept;

		smart_label_t *label;
		container_t *parent;

		void reset_selection(void);
		void set_selection(key_t key);
		void delete_selection(bool saveToCopybuffer);
		void ensure_on_cursor_screen(void);
		void update_selection(void);
		void set_text_finish(void);

		class drop_down_list_t : public window_component_t {
			private:
				int width;
				size_t current, top_idx;
				bool focus;
				text_field_t *field;

				string_list_t *completions;
				file_name_list_t::file_name_list_view_t *view;
			public:
				drop_down_list_t(t3_window_t *parent, t3_window_t *anchor, int _width, text_field_t *_field);
				~drop_down_list_t(void);
				virtual bool process_key(key_t key);
				virtual void set_position(optint top, optint left);
				virtual bool set_size(optint height, optint width);
				virtual void update_contents(void);
				virtual void set_focus(bool focus);
				virtual void show(void);
				virtual void hide(void);
				void update_view(void);
				void set_autocomplete(string_list_t *completions);
				bool has_items(void);
		};
		drop_down_list_t *drop_down_list;

	public:
		text_field_t(container_t *_parent);
		virtual ~text_field_t(void);
		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		virtual void show(void);
		virtual void hide(void);
		void set_text(const std::string *text);
		void set_text(const char *text);
		void set_autocomplete(string_list_t *_completions);
		void set_key_filter(key_t *keys, size_t nrOfKeys, bool accept);
		const std::string *get_text(void) const;
		const text_line_t *get_line(void) const;
		void set_label(smart_label_t *_label);
		virtual bool is_hotkey(key_t key);

	T3_WIDET_SIGNAL(activate, void);
	T3_WIDET_SIGNAL(lose_focus, void);
};

}; // namespace
#endif
