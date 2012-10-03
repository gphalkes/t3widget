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
#include <t3widget/widgets/widget.h>
#include <t3widget/widgets/expander.h>
#include <t3widget/widgets/widgetgroup.h>

namespace t3_widget {

/** A widget showing an expander, which allows hiding another widget. */
class T3_WIDGET_API expander_group_t : public widget_group_t {
	private:
		int expanded_child;

		void child_expanded(bool is_expanded, int child_idx);
	public:
		expander_group_t(void);

		/** Set the child widget. */
		virtual void add_child(widget_t *child);
		void collapse(void);

	T3_WIDGET_SIGNAL(expanded, void, bool);
};

}; // namespace
#endif
