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
#ifndef T3_WIDGET_EXPANDERGROUP_H
#define T3_WIDGET_EXPANDERGROUP_H

#include <deque>
#include <t3widget/widgets/expander.h>

namespace t3_widget {

/** A widget showing an expander, which allows hiding another widget. */
class T3_WIDGET_API expander_group_t {
	private:
		expander_t *expanded_widget;

		void widget_expanded(bool is_expanded, expander_t *source);
	public:
		expander_group_t(void);
		virtual ~expander_group_t(void);

		/** Set the child widget. */
		void add_expander(expander_t *expander);
		void collapse(void);

	T3_WIDGET_SIGNAL(expanded, void, bool);
};

}; // namespace
#endif
