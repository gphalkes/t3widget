/* Copyright (C) 2011,2018 G.P. Halkes
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
#include <deque>
#include <type_traits>

#include "t3widget/tinystring.h"
#include "t3widget/undo.h"
#include "t3widget/util.h"

namespace t3widget {
struct undo_list_t::implementation_t {
  std::deque<undo_t> list;
  std::deque<undo_t>::iterator current = list.end(), mark = list.end();
  bool mark_is_valid = true;
  bool mark_beyond_current = false;

  undo_t *add(undo_type_t type, text_coordinate_t coord) {
    if (list.empty()) {
      mark = list.emplace(list.end(), type, coord);
      current = list.end();
      return &*mark;
    }

    // Everything beyond current will be deleted, so mark will be invalid afterwards.
    if (mark_beyond_current) {
      mark_is_valid = false;
    }

    const bool mark_at_current = mark_is_valid && current == mark;
    if (current != list.end()) {
      list.erase(current, list.end());
    }
    auto iter = list.emplace(list.end(), type, coord);
    current = list.end();
    if (mark_at_current) {
      mark = iter;
    }
    return &*iter;
  }

  undo_t *back() {
    if (current == list.begin()) {
      return nullptr;
    }

    if (mark_is_valid && current == mark) {
      mark_beyond_current = true;
    }

    return &*--current;
  }

  undo_t *forward() {
    if (current == list.end()) {
      return nullptr;
    }
    undo_t *retval = &*current;
    ++current;

    if (mark_is_valid && mark_beyond_current && current == mark) {
      mark_beyond_current = false;
    }
    return retval;
  }

  void set_mark() {
    mark_is_valid = true;
    mark_beyond_current = false;
    mark = current;
  }

  bool is_at_mark() const { return mark_is_valid && mark == current; }
};

undo_list_t::undo_list_t() : impl(new implementation_t) {}
undo_list_t::~undo_list_t() {}

undo_t *undo_list_t::add(undo_type_t type, text_coordinate_t coord) {
  return impl->add(type, coord);
}

undo_t *undo_list_t::back() { return impl->back(); }

undo_t *undo_list_t::forward() { return impl->forward(); }

void undo_list_t::set_mark() { impl->set_mark(); }

bool undo_list_t::is_at_mark() const { return impl->is_at_mark(); }

#if 0
#ifdef DEBUG
#include "log.h"

static const char *undo_type_to_string[] = {
	"UNDO_NONE",
	"UNDO_DELETE",
	"UNDO_BACKSPACE",
	"UNDO_ADD",
	"UNDO_OVERWRITE",
    "UNDO_INDENT",
    "UNDO_UNINDENT",
    "UNDO_BLOCK_START",
	"UNDO_BLOCK_END",
	"UNDO_ADD_REDO",
	"UNDO_BACKSPACE_REDO",
	"UNDO_OVERWRITE_REDO",
	"UNDO_BLOCK_START_REDO",
	"UNDO_BLOCK_END_REDO"
};

void undo_list_t::dump() {
	undo_t *ptr;

	ptr = head;

	while (ptr != nullptr) {
		text_coordinate_t start = ptr->get_start();

		lprintf("undo_t:%s%c(%d,%d) %s: '", ptr == current ? "->" : "  ", mark_is_valid && ptr == mark ? '@' : ' ',
			start.line, start.pos, undo_type_to_string[ptr->get_type_t()]);
		if (ptr->get_text() != nullptr)
			ldumpstr(ptr->get_text()->getData()->data(), ptr->get_text()->getLength());
		lprintf("'\n");

		if (ptr->get_replacement() != nullptr) {
			lprintf("        Replaced by: '");
			ldumpstr(ptr->get_replacement()->getData()->data(), ptr->get_replacement()->getLength());
			lprintf("'\n");
		}
		ptr = ptr->next;
	}
	if (current == nullptr)
		lprintf("undo_t:-> END\n");
}
#endif
#endif

undo_type_t undo_t::redo_map[] = {
    UNDO_NONE,     UNDO_ADD,    UNDO_BACKSPACE_REDO,   UNDO_ADD_REDO,      UNDO_OVERWRITE_REDO,
    UNDO_UNINDENT, UNDO_INDENT, UNDO_BLOCK_START_REDO, UNDO_BLOCK_END_REDO};

undo_type_t undo_t::get_type() const { return type; }
undo_type_t undo_t::get_redo_type() const { return redo_map[type]; }
text_coordinate_t undo_t::get_start() { return start; }
void undo_t::add_newline() { text.append(1, '\n'); }
tiny_string_t *undo_t::get_text() { return &text; }
void undo_t::minimize() { text.shrink_to_fit(); }

}  // namespace t3widget
