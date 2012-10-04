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

expander_group_t::expander_group_t(void) : expanded_widget(NULL) {}

void expander_group_t::add_expander(expander_t *expander) {
	if (expander == NULL)
		return;
	expander->connect_expanded(sigc::bind(sigc::mem_fun(this, &expander_group_t::widget_expanded), expander));
	expander->collapse();
}

void expander_group_t::widget_expanded(bool is_expanded, expander_t *source) {
	if (is_expanded) {
		if (expanded_widget != NULL)
			expanded_widget->collapse();
		expanded_widget = source;
	} else {
		if (source == expanded_widget)
			expanded_widget = NULL;
	}
	expanded(is_expanded);
}

void expander_group_t::collapse(void) {
	if (expanded_widget != NULL) {
		expanded_widget->collapse();
		expanded_widget = NULL;
		expanded(false);
	}
}

}; // namespace
