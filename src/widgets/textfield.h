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
#ifndef TEXTFIELD_H
#define TEXTFIELD_H

#include <string>

class text_field_t;

#include "widgets/widgets.h"
#include "lines.h"
#include "widgets/contentlist.h"

class text_field_t : public widget_t {
	protected:
		int width,
			pos,
			screen_pos,
			leftcol,
			selection_start_pos,
			selection_end_pos;

		SelectionMode selection_mode;
		bool focus, in_drop_down_list, dont_select_on_focus;
		enum {
			NO_REPAINT,
			REPAINT_EDIT,
			REPAINT_SET,
			REPAINT_RESIZE,
			REPAINT_OTHER
		} need_repaint;

		line_t *line;
		const key_t *filter_keys;
		size_t filter_keys_size;
		bool filter_keys_accept;

		smart_label_t *label;
		t3_window_t *parent_window;

		void init(window_component_t *parent, window_component_t *anchor, int top, int left, int relation);
		void reset_selection(void);
		void set_selection(key_t key);
		void delete_selection(bool saveToCopybuffer);
		void ensure_on_cursor_screen(void);
		void update_selection(void);
		void set_text_finish(void);

		class drop_down_list_t : public window_component_t {
			private:
				t3_window_t *window;
				int width;
				size_t current, top_idx;
				bool focus;
				text_field_t *field;

				string_list_t *completions;
				file_name_list_t::file_name_list_view_t *view;
			public:
				drop_down_list_t(t3_window_t *parent, t3_window_t *anchor, int _width, text_field_t *_field);
				~drop_down_list_t(void);
				virtual void process_key(key_t key);
				virtual bool resize(optint height, optint _width, optint top, optint left);
				virtual void update_contents(void);
				virtual void set_focus(bool focus);
				virtual void show(void);
				virtual void hide(void);
				virtual t3_window_t *get_draw_window(void) { return window; }
				void update_view(void);
				void set_autocomplete(string_list_t *completions);
				bool has_items(void);
		};
		drop_down_list_t *drop_down_list;

	public:
		text_field_t(window_component_t *parent, int top, int left, int _width);
		text_field_t(window_component_t *parent, window_component_t *anchor, int top, int left, int _width, int relation);
		virtual ~text_field_t(void);
		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint _width, optint top, optint _left);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		virtual void show(void);
		virtual void hide(void);
		void set_text(const std::string *text);
		void set_text(const char *text);
		void set_autocomplete(string_list_t *_completions);
		void set_key_filter(key_t *keys, size_t nrOfKeys, bool accept);
		const std::string *get_text(void) const;
		const line_t *get_line(void) const;
		void set_label(smart_label_t *_label);
		virtual bool is_hotkey(key_t key);

	SIGNAL(activate, void);
	SIGNAL(lose_focus, void);
	SIGNAL(move_focus_up, void);
	SIGNAL(move_focus_down, void);
};


#endif
