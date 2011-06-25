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
#include <stdint.h>
#include <t3unicode/unicode.h>
#include <t3window/window.h>
#include "widgets/widget.h"
#include "widgets/smartlabel.h"
#include "textline.h"
#include "util.h"
#include "colorscheme.h"

using namespace std;
namespace t3_widget {

smart_label_text_t::smart_label_text_t(const char *spec, bool _add_colon) : add_colon(_add_colon), underlined(false), hotkey(0) {
	text_line_t *line;
	char *underline_ptr;

	text_length = strlen(spec);
	if ((text = strdup(spec)) == NULL)
		throw bad_alloc();

	if ((underline_ptr = strchr(text, '_')) != NULL) {
		size_t src_size;

		underlined = true;
		underline_start = underline_ptr - text;
		memmove(underline_ptr, underline_ptr + 1, text_length - underline_start);
		text_length--;

		hotkey = t3_unicode_casefold_single(t3_unicode_get(underline_ptr, &src_size));

		//FIXME: an alloc error here will cause a leak of the allocated 'text' var
		line = new text_line_t(text, text_length);
		underline_length = line->adjust_position(underline_start, 1) - underline_start;
		delete line;
	}
}

smart_label_text_t::smart_label_text_t(smart_label_text_t *other):
	add_colon(other->add_colon), text(other->text), underline_start(other->underline_start),
	underline_length(other->underline_length), text_length(other->text_length), underlined(other->underlined),
	hotkey(other->hotkey)
{
	delete other;
}

smart_label_text_t::~smart_label_text_t(void) {
	free(text);
}

void smart_label_text_t::draw(t3_window_t *window, int attr, bool selected) {
	if (!underlined) {
		t3_win_addnstr(window, text, text_length, attr);
	} else {
		t3_win_addnstr(window, text, underline_start, attr);
		t3_win_addnstr(window, text + underline_start, underline_length,
			selected ? attr : t3_term_combine_attrs(attributes.highlight, attr));
		t3_win_addnstr(window, text + underline_start + underline_length, text_length - underline_start - underline_length, attr);
	}
	if (add_colon)
		t3_win_addch(window, ':', attr);
}

int smart_label_text_t::get_width(void) {
	return t3_term_strwidth(text) + (add_colon ? 1 : 0);
}

bool smart_label_text_t::is_hotkey(key_t key) {
	if (hotkey == 0)
		return false;

	return (key_t) t3_unicode_casefold_single(key & UINT32_C(0x1fffff)) == hotkey;
}

//======= smart_label_t =======

smart_label_t::smart_label_t(smart_label_text_t *spec) :
	smart_label_text_t(spec), widget_t(1, get_width()) {}

smart_label_t::smart_label_t(const char *spec, bool _add_colon) :
	smart_label_text_t(spec, _add_colon), widget_t(1, get_width()) {}

bool smart_label_t::process_key(key_t key) { (void) key; return false; }

bool smart_label_t::set_size(optint height, optint width) {
	(void) height;
	(void) width;
	return true;
}

void smart_label_t::update_contents(void) {
	if (!redraw)
		return;
	redraw = false;
	t3_win_set_paint(window, 0, 0);
	draw(window, attributes.dialog);
}

void smart_label_t::set_focus(bool focus) { (void) focus; }

bool smart_label_t::is_hotkey(key_t key) {
	return smart_label_text_t::is_hotkey(key);
}

bool smart_label_t::accepts_focus(void) { return false; }

}; // namespace
