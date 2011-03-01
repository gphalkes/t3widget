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
#include <climits>
#include <cstring>
#include <vector>
#include <algorithm>
#include "unicode.h"
#include "widgets/widgets.h"
#include "lines.h"
#include "util.h"
#include "window.h"
#include "colorscheme.h"

using namespace std;

smart_label_text_t::smart_label_text_t(const char *spec, bool _addColon) : add_colon(_addColon) {
	line_t *line;
	size_t i;
	size_t spec_length = strlen(spec);

	underline_start = spec_length;
	for (i = 0; i < spec_length; i++) {
		if (spec[i] == '_')
			underline_start = i;
		if (spec[i] == ';')
			break;
	}
	text_length = i;
	text = new char[text_length + 1];
	strncpy(text, spec, text_length);
	text[text_length] = 0;
	if (underline_start < text_length) {
		memmove(text + underline_start, text + underline_start + 1, text_length - underline_start);
		text_length--;
	}

	line = new line_t(text, text_length);
	underline_length = line->adjust_position(underline_start, 1) - underline_start;
	delete line;

	hotkeys = NULL;
	i++;
	if (i < spec_length) {
		key_t next;
		vector<key_t> hotkey_vector;
		spec += i;

		while (*spec != 0 && (next = t3_getuc(spec, &i)) != 0xFFFD) {
			hotkey_vector.push_back(next);
			spec += i;
		}

		hotkeys = new key_t[hotkey_vector.size() + 1];
		copy(hotkey_vector.begin(), hotkey_vector.end(), hotkeys);
		hotkeys[hotkey_vector.size()] = 0;
	}
}

smart_label_text_t::smart_label_text_t(smart_label_text_t *other):
	add_colon(other->add_colon), text(other->text), underline_start(other->underline_start),
	underline_length(other->underline_length), text_length(other->text_length), underlined(other->underlined),
	hotkeys(other->hotkeys)
{
	delete other;
}

void smart_label_text_t::draw(t3_window_t *window, int attr) {
	if (underline_start > text_length) {
		t3_win_addnstr(window, text, text_length, attr);
	} else {
		t3_win_addnstr(window, text, underline_start, attr);
		t3_win_addnstr(window, text + underline_start, underline_length, t3_term_combine_attrs(colors.highlight_attrs, attr));
		t3_win_addnstr(window, text + underline_start + underline_length, text_length - underline_start - underline_length, attr);
	}
	if (add_colon)
		t3_win_addch(window, ':', attr);
}

int smart_label_text_t::get_width(void) {
	return t3_term_strwidth(text) + (add_colon ? 1 : 0);
}

bool smart_label_text_t::is_hotkey(key_t key) {
	int i;

	if (hotkeys == NULL)
		return false;

	for (i = 0; hotkeys[i] != 0; i++)
		if (key == hotkeys[i])
			return true;
	return false;
}

smart_label_t::smart_label_t(window_component_t *parent, int top, int left, smart_label_text_t *spec) : smart_label_text_t(spec) {
	init(parent, parent, top, left, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
}

smart_label_t::smart_label_t(window_component_t *parent, int top, int left, const char *spec, bool _addColon) :
	smart_label_text_t(spec, _addColon)
{
	init(parent, parent, top, left, T3_PARENT(T3_ANCHOR_TOPLEFT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
}

smart_label_t::smart_label_t(window_component_t *parent, window_component_t *anchor,
	int top, int left, int relation, smart_label_text_t *spec) : smart_label_text_t(spec)
{
	init(parent, anchor, top, left, relation);
}

smart_label_t::smart_label_t(window_component_t *parent, window_component_t *anchor,
	int top, int left, int relation, const char *spec, bool _addColon) : smart_label_text_t(spec, _addColon)
{
	init(parent, anchor, top, left, relation);
}

void smart_label_t::init(window_component_t *parent, window_component_t *anchor, int top, int left, int relation) {
	if (anchor == NULL)
		anchor = parent;

	if ((window = t3_win_new_relative(parent->get_draw_window(), 1, get_width(), top, left, 0, anchor->get_draw_window(), relation)) == NULL)
		throw(-1);

	t3_win_show(window);
}

void smart_label_t::process_key(key_t key) { (void) key; }

bool smart_label_t::resize(optint height, optint width, optint top, optint left) {
	(void) height;
	(void) width;

	if (!top.is_valid())
		top = t3_win_get_y(window);
	if (!left.is_valid())
		left = t3_win_get_x(window);

	t3_win_move(window, top, left);
	return true;
}

void smart_label_t::update_contents(void) {
	t3_win_set_paint(window, 0, 0);
	draw(window, 0);
}

void smart_label_t::set_focus(bool focus) { (void) focus; }

bool smart_label_t::is_hotkey(key_t key) {
	return smart_label_text_t::is_hotkey(key);
}

bool smart_label_t::accepts_focus(void) { return false; }
