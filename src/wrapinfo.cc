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

#include "wrapinfo.h"

namespace t3_widget {

wrap_info_t::wrap_info_t(int width, int _tabsize) : text(NULL), size(0), tabsize(_tabsize), wrap_width(width) {}

wrap_info_t::~wrap_info_t(void) {
	rewrap_connection.disconnect();
}

int wrap_info_t::get_size() { return size; }

void wrap_info_t::delete_lines(int first, int last) {
	for (wrap_data_t::iterator iter = wrap_data.begin() + first; iter != wrap_data.begin() + last; iter++) {
		size -= (*iter)->size();
		delete *iter;
	}
	wrap_data.erase(wrap_data.begin() + first, wrap_data.begin() + last);
}

void wrap_info_t::insert_lines(int first, int last) {
	int i;
	for (i = first; i < last; i++) {
		wrap_data.insert(wrap_data.begin() + i, new wrap_points_t());
		// Ensure that the list of break positions contains at least the start position.
		wrap_data[i]->push_back(0);
		size++;
		rewrap_line(i, 0, true);
	}
}

void wrap_info_t::rewrap_line(int line, int pos, bool local) {
	text_line_t::break_pos_t break_pos;
	size_t i;

	/* The list of break positions always contains the start position (0). */

	for (i = wrap_data[line]->size() - 1; i > 0 && (*wrap_data[line])[i] > pos; i++) {}

	if (local) {
		break_pos = text->lines[line]->find_next_break_pos((*wrap_data[line])[i], wrap_width, tabsize);
		if (i < wrap_data[line]->size() - 1 && break_pos.pos == (*wrap_data[line])[i + 1])
			return;
	}

	/* Keep it simple: subtract the full size here, and add the full size again
	   when we are done rewrapping. */
	size -= wrap_data[line]->size();
	wrap_data[line]->erase(wrap_data[line]->begin() + i + 1, wrap_data[line]->end());

	while (1) {
		break_pos = text->lines[line]->find_next_break_pos((*wrap_data[line])[i], wrap_width, tabsize);
		if (break_pos.pos > 0)
			wrap_data[line]->push_back(break_pos.pos);
		else
			break;
	}
	size += wrap_data[line]->size();
}

void wrap_info_t::rewrap_all(void) {
	for (size_t i = 0; i < wrap_data.size(); i++)
		rewrap_line(i, 0, false);
}

void wrap_info_t::set_wrap_width(int width) {
	if (width == wrap_width)
		return;
	wrap_width = width;
	if (text != NULL)
		rewrap_all();
}

void wrap_info_t::set_tabsize(int _tabsize) {
	if (_tabsize == tabsize)
		return;
	tabsize = _tabsize;
	if (text != NULL)
		rewrap_all();
}

void wrap_info_t::set_text_buffer(text_buffer_t *_text) {
	rewrap_connection.disconnect();

	text = _text;
	if (_text == NULL)
		return;

	rewrap_connection = text->connect_rewrap_required(sigc::mem_fun(this, &wrap_info_t::rewrap));

	if (wrap_data.size() > text->lines.size())
		delete_lines(text->lines.size(), wrap_data.size());

	for (size_t i = 0; i < wrap_data.size(); i++)
		rewrap_line(i, 0, false);

	if (wrap_data.size() < text->lines.size())
		insert_lines(wrap_data.size(), text->lines.size());
}

void wrap_info_t::rewrap(rewrap_type_t type, int a, int b) {
	switch (type) {
		case rewrap_type_t::REWRAP_ALL:
			rewrap_all();
			break;
		case rewrap_type_t::REWRAP_LINE:
			rewrap_line(a, b, false);
			break;
		case rewrap_type_t::REWRAP_LINE_LOCAL:
			rewrap_line(a, b, true);
			break;
		case rewrap_type_t::INSERT_LINES:
			insert_lines(a, b);
			break;
		case rewrap_type_t::DELETE_LINES:
			delete_lines(a, b);
			break;
	}
}

}; // namespace
