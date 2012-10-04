/* Copyright (C) 2012 G.P. Halkes
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
#include "widgets/expandergroup.h"
#include "log.h"

using namespace std;
namespace t3_widget {

expander_group_t::expander_group_t(void) : expanded_child(-1) {}

void expander_group_t::add_child(widget_t *child) {
	expander_t *expander_child = dynamic_cast<expander_t *>(child);

	if (expander_child == NULL)
		return;
	expander_child->connect_expanded(sigc::bind(sigc::mem_fun(this, &expander_group_t::child_expanded), (int) impl->children.size()));
	expander_child->collapse();
	widget_vgroup_t::add_child(child);
}

void expander_group_t::child_expanded(bool is_expanded, int child_idx) {
	if (is_expanded) {
		if (expanded_child >= 0) {
			expander_t *expander_child = dynamic_cast<expander_t *>(impl->children[expanded_child]);
			if (expander_child != NULL)
				expander_child->collapse();
		}
		expanded_child = child_idx;
	} else {
		expanded_child = -1;
	}
	set_size(None, None);
	expanded(is_expanded);
}

void expander_group_t::collapse(void) {
	if (expanded_child >= 0) {
		expander_t *expander_child = dynamic_cast<expander_t *>(impl->children[expanded_child]);
		if (expander_child == NULL)
			return;
		expander_child->collapse();
	}
}

}; // namespace
