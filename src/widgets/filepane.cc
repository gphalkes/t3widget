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
#include "widgets/filepane.h"
#include "main.h"

using namespace std;
namespace t3_widget {

//FIXME: we could use some optimization for update_column_widths. Current use is simple but calls to often.
/* FIXME: we should not distribute left-over space among shown columns, but show partial column instead
	this is more intuitive for the user. */
file_pane_t::file_pane_t(void) : widget_t(3, 3), impl(new implementation_t())
{
	set_widget_parent(&impl->scrollbar);
	impl->scrollbar.set_anchor(this, T3_PARENT(T3_ANCHOR_BOTTOMLEFT) | T3_CHILD(T3_ANCHOR_BOTTOMLEFT));
	impl->scrollbar.connect_clicked(sigc::mem_fun(this, &file_pane_t::scrollbar_clicked));
	impl->search_panel = new search_panel_t(this);
}

file_pane_t::~file_pane_t(void) {
	impl->content_changed_connection.disconnect();
}

void file_pane_t::set_text_field(text_field_t *_field) {
	impl->field = _field;
}

void file_pane_t::ensure_cursor_on_screen(void) {
	size_t old_top_idx = impl->top_idx;
	int height;

	if (impl->file_list == NULL)
		return;

	height = t3_win_get_height(window) - 1;

	while (impl->current >= impl->top_idx + impl->columns_visible * height)
		impl->top_idx += height;
	while (impl->current < impl->top_idx && impl->top_idx > (size_t) height)
		impl->top_idx -= height;
	if (impl->top_idx > impl->current)
		impl->top_idx = 0;
	if (impl->top_idx != old_top_idx) {
		update_column_widths();
		ensure_cursor_on_screen();
		impl->scrollbar.set_parameters(impl->scrollbar_range, impl->top_idx, impl->columns_visible * height);
	}
}

bool file_pane_t::process_key(key_t key) {
	int height;
	if (impl->file_list == NULL)
		return false;

	if (impl->search_panel_shown) {
		if (impl->search_panel->process_key(key))
			return true;
	}

	switch (key) {
		case EKEY_DOWN:
			if (impl->current + 1 >= impl->file_list->size())
				return true;
			impl->current++;
			redraw = true;
			break;
		case EKEY_UP:
			if (impl->current == 0)
				return true;
			impl->current--;
			redraw = true;
			break;
		case EKEY_RIGHT:
			height = t3_win_get_height(window) - 1;
			if (impl->current + height >= impl->file_list->size()) {
				if (impl->file_list->size() == 0)
					return true;
				impl->current = impl->file_list->size() - 1;
			} else {
				impl->current += height;
			}
			redraw = true;
			break;
		case EKEY_LEFT:
			height = t3_win_get_height(window) - 1;
			if (impl->current < (size_t) height)
				impl->current = 0;
			else
				impl->current -= height;
			redraw = true;
			break;
		case EKEY_END:
			impl->current = impl->file_list->size() - 1;
			redraw = true;
			break;
		case EKEY_HOME:
			impl->current = 0;
			redraw = true;
			break;
		case EKEY_PGDN:
			height = t3_win_get_height(window) - 1;
			if (impl->current + 2 * height >= impl->file_list->size()) {
				impl->current = impl->file_list->size() - 1;
			} else {
				impl->current += 2 * height;
				impl->top_idx += 2 * height;
			}
			redraw = true;
			break;
		case EKEY_PGUP:
			height = t3_win_get_height(window) - 1;
			if (impl->current < (size_t) 2 * height) {
				impl->current = 0;
			} else {
				impl->current -= 2 * height;
				if (impl->top_idx < (size_t) 2 * height)
					impl->top_idx = 0;
				else
					impl->top_idx -= 2 * height;
			}
			redraw = true;
			break;
		case EKEY_NL:
			activate(impl->file_list->get_fs_name(impl->current));
			return true;
		default:
			if (key >= 32 && key < 0x110000) {
				impl->search_panel->show();
				impl->search_panel->process_key(key);
				return true;
			}
			return false;
	}
	if (impl->file_list->size() != 0) {
		if (impl->field != NULL)
			impl->field->set_text((*impl->file_list)[impl->current]->c_str());
		ensure_cursor_on_screen();
	}
	return true;
}

bool file_pane_t::set_size(optint height, optint width) {
	bool result;
	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_height(window);
	result = t3_win_resize(window, height, width);
	result &= impl->scrollbar.set_size(None, width);

	height = height - 1;
	if (impl->file_list != NULL && impl->file_list->size() != 0) {
		update_column_widths();
		impl->scrollbar_range = ((impl->file_list->size() + height - 1) / height) * height;
		ensure_cursor_on_screen();
		impl->scrollbar.set_parameters(impl->scrollbar_range, impl->top_idx, impl->columns_visible * height);
	}
	redraw = true;
	return result;
}

void file_pane_t::draw_line(int idx, bool selected) {
	if ((size_t) idx < impl->top_idx || (size_t) idx >= impl->file_list->size())
		return;

	int column;
	int height = t3_win_get_height(window) - 1;
	text_line_t line((*impl->file_list)[idx]);
	bool is_dir = impl->file_list->is_dir(idx);
	text_line_t::paint_info_t info;

	idx -= impl->top_idx;
	column = idx / height;
	idx %= height;

	t3_win_set_paint(window, idx, impl->column_positions[column]);
	t3_win_addch(window, is_dir ? '/' : ' ', selected ? attributes.dialog_selected : 0);

	info.start = 0;
	info.leftcol = 0;
	info.max = INT_MAX;
	info.size = impl->column_widths[column];
	info.tabsize = 0;
	info.flags = text_line_t::SPACECLEAR | text_line_t::TAB_AS_CONTROL | (selected ? text_line_t::EXTEND_SELECTION : 0);
	info.selection_start = -1;
	info.selection_end = selected ? INT_MAX : -1;
	info.cursor = -1;
	info.normal_attr = attributes.dialog;
	info.selected_attr = attributes.dialog_selected;

	line.paint_line(window, &info);
}

void file_pane_t::update_contents(void) {
	size_t max_idx, i;
	int height;

	impl->search_panel->update_contents();

	if (!redraw)
		return;
	redraw = false;

	t3_win_set_default_attrs(window, attributes.dialog);

	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	if (impl->file_list == NULL)
		return;

	height = t3_win_get_height(window) - 1;
	for (i = impl->top_idx, max_idx = min(impl->top_idx + impl->columns_visible * height, impl->file_list->size()); i < max_idx; i++)
		draw_line(i, impl->focus && i == impl->current);

	impl->scrollbar.update_contents();
}

void file_pane_t::set_focus(focus_t _focus) {
	impl->focus = _focus;
	if (impl->file_list != NULL)
		draw_line(impl->current, impl->focus);
}

void file_pane_t::set_child_focus(window_component_t *target) {
	(void) target;
	set_focus(window_component_t::FOCUS_SET);
}

bool file_pane_t::is_child(window_component_t *widget) {
	return widget == &impl->scrollbar;
}

bool file_pane_t::process_mouse_event(mouse_event_t event) {
	if (event.window != window)
		return true;
	if ((event.type == EMOUSE_BUTTON_RELEASE && (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT)) ||
			(event.type == EMOUSE_BUTTON_PRESS && (event.button_state & EMOUSE_BUTTON_LEFT))) {
		int column;
		size_t idx;

		if ((event.button_state & (EMOUSE_BUTTON_LEFT | EMOUSE_DOUBLE_CLICKED_LEFT)) == 0 || impl->columns_visible == 0)
			return true;

		for (column = 1; column < impl->columns_visible && impl->column_positions[column] < event.x; column++) {}
		column--;
		idx = column * (t3_win_get_height(window) - 1) + event.y + impl->top_idx;
		if (idx > impl->file_list->size())
			return true;
		if (event.button_state & EMOUSE_DOUBLE_CLICKED_LEFT) {
			activate(impl->file_list->get_fs_name(impl->current));
		} else if (event.button_state & EMOUSE_BUTTON_LEFT) {
			impl->current = idx;
			if (impl->field != NULL)
				impl->field->set_text((*impl->file_list)[impl->current]->c_str());
			redraw = true;
			return true;
		}
	}
	return true;
}

void file_pane_t::reset(void) {
	impl->top_idx = 0;
	impl->current = 0;
}

void file_pane_t::set_file_list(file_list_t *_file_list) {
	if (impl->file_list != NULL)
		impl->content_changed_connection.disconnect();

	impl->file_list = _file_list;
	impl->content_changed_connection = impl->file_list->connect_content_changed(sigc::mem_fun(this, &file_pane_t::content_changed));
	impl->top_idx = 0;
	content_changed();
	redraw = true;
}

void file_pane_t::set_file(const string *name) {
	for (impl->current = 0; impl->current < impl->file_list->size(); impl->current++) {
		if (name->compare(*(*impl->file_list)[impl->current]) == 0)
			break;
	}
	if (impl->current == impl->file_list->size())
		impl->current = 0;

	ensure_cursor_on_screen();
}

void file_pane_t::update_column_width(int column, int start) {
	int height = t3_win_get_height(window) - 1;
	impl->column_widths[column] = 0;
	for (int i = 0; i < height && start + i < (int) impl->file_list->size(); i++)
		impl->column_widths[column] = max(impl->column_widths[column], t3_term_strwidth((*impl->file_list)[i + start]->c_str()));
}

void file_pane_t::update_column_widths(void) {
	int i, sum_width;
	int height = t3_win_get_height(window) - 1;
	int width = t3_win_get_width(window);

	if (impl->file_list == NULL)
		return;

	for (i = 0, sum_width = 0; i < _T3_WDIGET_FP_MAX_COLUMNS && sum_width < width &&
			impl->top_idx + i * height < impl->file_list->size(); i++)
	{
		update_column_width(i, impl->top_idx + i * height);
		sum_width += impl->column_widths[i] + 2;
	}

	impl->columns_visible = i;
	if (sum_width > width) {
		if (i > 1) {
			sum_width -= impl->column_widths[i - 1] + 2;
			impl->columns_visible = i - 1;
		} else {
			impl->column_widths[0] = width;
			sum_width = width;
		}
	}
	if (impl->columns_visible == 0)
		impl->columns_visible = 1;

	for (i = 0; i < impl->columns_visible; i++)
		impl->column_widths[i] += (width - sum_width) / impl->columns_visible;
	sum_width += impl->columns_visible * ((width - sum_width) / impl->columns_visible);
	for (i = 0; i < impl->columns_visible; i++)
		impl->column_widths[i]++;
	impl->column_positions[0] = 0;
	for (i = 1; i < impl->columns_visible; i++)
		impl->column_positions[i] = impl->column_positions[i - 1] + impl->column_widths[i - 1] + 1;
}

void file_pane_t::content_changed(void) {
	int height = t3_win_get_height(window) - 1;

	impl->top_idx = 0;
	update_column_widths();
	impl->scrollbar_range = ((impl->file_list->size() + height - 1) / height) * height;
	impl->scrollbar.set_parameters(impl->scrollbar_range, 0, impl->columns_visible * height);
	redraw = true;
}

void file_pane_t::scrollbar_clicked(scrollbar_t::step_t step) {
	int height = t3_win_get_height(window) - 1;
	if (impl->file_list == NULL)
		return;

	/* FIXME: for now, clicking the empty area of the scrollbar will simply
		jump one column, because doing it differently is too tricky (for now). */
	if (step == scrollbar_t::FWD_SMALL || step == scrollbar_t::FWD_MEDIUM || step == scrollbar_t::FWD_PAGE) {
		if (impl->top_idx + impl->columns_visible * height >= impl->file_list->size())
			return;
		impl->top_idx += height;
	} else if (step == scrollbar_t::FWD_PAGE) {
		if (impl->current + 2 * height >= impl->file_list->size()) {
			impl->current = impl->file_list->size() - 1;
		} else {
			impl->current += 2 * height;
			impl->top_idx += 2 * height;
		}
	} else if (step == scrollbar_t::BACK_SMALL || step == scrollbar_t::BACK_MEDIUM || step == scrollbar_t::BACK_PAGE) {
		if (impl->top_idx == 0)
			return;
		if (impl->top_idx < (size_t) height)
			impl->top_idx = 0;
		else
			impl->top_idx -= height;
	} else if (step == scrollbar_t::BACK_PAGE) {
		if (impl->current < (size_t) 2 * height) {
			impl->current = 0;
		} else {
			impl->current -= 2 * height;
			if (impl->top_idx < (size_t) 2 * height)
				impl->top_idx = 0;
			else
				impl->top_idx -= 2 * height;
		}
	}

	update_column_widths();
	if (impl->current < impl->top_idx)
		impl->current = impl->top_idx;
	else if (impl->current >= impl->file_list->size())
		impl->current = impl->file_list->size() - 1;
	else if (impl->current >= impl->top_idx + impl->columns_visible * height)
		impl->current = impl->top_idx + impl->columns_visible * height - 1;
	impl->scrollbar.set_parameters(impl->scrollbar_range, impl->top_idx, impl->columns_visible * height);
	redraw = true;

	if (impl->file_list->size() != 0 && impl->field != NULL)
		impl->field->set_text((*impl->file_list)[impl->current]->c_str());
}

void file_pane_t::search(const std::string *text) {
	size_t i, j;
	size_t longest_match = 0;
	size_t longest_match_idx = 0;

	for (i = 0; i < impl->file_list->size(); i++) {
		for (j = 0; j < (*impl->file_list)[i]->size() && j < text->size(); j++) {
			if ((*(*impl->file_list)[i])[j] != (*text)[j])
				break;
		}
		// Adjust match length to start of UTF-8 character.
		while (j > 0 && ((*text)[j] & 0xC0) == 0x80)
			j--;
		if (j > longest_match) {
			longest_match = j;
			longest_match_idx = i;
		}
	}
	if (longest_match > 0 && impl->current != longest_match_idx) {
		impl->current = longest_match_idx;
		redraw = true;
		ensure_cursor_on_screen();
	}
}

//============================ search_panel_t ==========================
#define SEARCH_PANEL_WIDTH 12
file_pane_t::search_panel_t::search_panel_t(file_pane_t *_parent) : parent(_parent), redraw(true) {
	if ((window = t3_win_new(NULL, 3, SEARCH_PANEL_WIDTH, -1, -12, INT_MIN)) == NULL)
		throw bad_alloc();
	t3_win_set_anchor(window, parent->get_base_window(), T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	if ((shadow_window = t3_win_new(NULL, 3, SEARCH_PANEL_WIDTH, 0, -11, INT_MIN + 1)) == NULL)
		throw bad_alloc();
	t3_win_set_anchor(shadow_window, parent->get_base_window(), T3_PARENT(T3_ANCHOR_BOTTOMRIGHT) | T3_CHILD(T3_ANCHOR_TOPRIGHT));
	register_mouse_target(window);
}

bool file_pane_t::search_panel_t::process_key(key_t key) {
	switch (key) {
		case EKEY_BS:
			text.backspace_char(text.get_length(), NULL);
			redraw = true;
			return true;
		case EKEY_ESC:
			hide();
			return true;
		case EKEY_F9:
			insert_char_dialog->center_over(parent);
			insert_char_dialog->reset();
			insert_char_dialog->show();
			return true;
		default:
			if (key < 32)
				break;

			key &= ~EKEY_PROTECT;
			if (key == 10)
				break;

			if (key >= 0x110000)
				break;
			text.append_char(key, NULL);
			parent->search(text.get_data());
			redraw = true;
			return true;
	}
	hide();
	return false;
}

void file_pane_t::search_panel_t::set_position(optint top, optint left) {
	(void) top;
	(void) left;
}

bool file_pane_t::search_panel_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

void file_pane_t::search_panel_t::update_contents(void) {
	if (!redraw)
		return;

	t3_win_set_default_attrs(window, attributes.dialog);
	t3_win_set_default_attrs(shadow_window, attributes.shadow);

	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);

	text_line_t::paint_info_t paint_info;
	int text_width = text.calculate_screen_width(0, INT_MAX, 0);

	paint_info.start = 0;
	paint_info.leftcol = SEARCH_PANEL_WIDTH - 2 < text_width ? text_width - SEARCH_PANEL_WIDTH + 2 : 0;
	paint_info.size = SEARCH_PANEL_WIDTH - 2;
	paint_info.max = INT_MAX;
	paint_info.tabsize = 0;
	paint_info.flags = text_line_t::TAB_AS_CONTROL;
	paint_info.selection_start = -1;
	paint_info.selection_end = -1;
	paint_info.cursor = -1;
	paint_info.normal_attr = 0;
	paint_info.selected_attr = 0;

	t3_win_set_paint(window, 1, 1);
	text.paint_line(window, &paint_info);
	t3_win_box(window, 0, 0, t3_win_get_height(window), t3_win_get_width(window), 0);

	int x = t3_win_get_width(shadow_window) - 1;
	for (int i = t3_win_get_height(shadow_window) - 1; i > 0; i--) {
		t3_win_set_paint(shadow_window, i - 1, x);
		t3_win_addch(shadow_window, ' ', 0);
	}
	t3_win_set_paint(shadow_window, t3_win_get_height(shadow_window) - 1, 0);
	t3_win_addchrep(shadow_window, ' ', 0, t3_win_get_width(shadow_window));

	redraw = false;
}

void file_pane_t::search_panel_t::set_focus(focus_t _focus) {
	(void) _focus;
}

void file_pane_t::search_panel_t::show(void) {
	text.set_text("");
	redraw = true;
	t3_win_show(window);
	t3_win_show(shadow_window);
	parent->impl->search_panel_shown = true;
	grab_mouse();
}

void file_pane_t::search_panel_t::hide(void) {
	t3_win_hide(window);
	t3_win_hide(shadow_window);
	parent->impl->search_panel_shown = false;
	release_mouse_grab();
}

void file_pane_t::search_panel_t::force_redraw(void) {
	redraw = true;
}

bool file_pane_t::search_panel_t::process_mouse_event(mouse_event_t event) {
	if ((event.type & EMOUSE_OUTSIDE_GRAB) && (event.type & ~EMOUSE_OUTSIDE_GRAB) == EMOUSE_BUTTON_PRESS) {
		hide();
		if (event.window == parent->window()) {
			event.type &= ~EMOUSE_OUTSIDE_GRAB;
			event.x += t3_win_get_abs_x(window) - t3_win_get_abs_x(parent->get_base_window());
			event.y += t3_win_get_abs_y(window) - t3_win_get_abs_y(parent->get_base_window());
			parent->process_mouse_event(event);
		}
	}
	return true;
}

}; // namespace
