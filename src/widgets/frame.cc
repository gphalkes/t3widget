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
#include "widgets/widgets.h"

frame_t::frame_t(container_t *parent, widget_t *_child, frame_dimension_t _dimension) : widget_t(parent, 3, 3),
		dimension(_dimension), child(_child)
{
	int child_top = 1, child_left = 1;
	if (dimension & COVER_TOP)
		child_top = 0;
	if (dimension & COVER_LEFT)
		child_left = 0;
	child->set_anchor(this, 0);
	child->set_position(child_top, child_left);
	set_size(3, 3);
}

bool frame_t::process_key(key_t key) { return child->process_key(key); }
void frame_t::update_contents(void) { child->update_contents(); }
void frame_t::set_focus(bool focus) { child->set_focus(focus); }

bool frame_t::set_size(optint height, optint width) {
	int child_height, child_width;
	bool result;

	if (!height.is_valid())
		height = t3_win_get_height(window);
	if (!width.is_valid())
		width = t3_win_get_height(window);

	result = t3_win_resize(window, height, width);
	t3_win_set_paint(window, 0, 0);
	t3_win_clrtobot(window);
	t3_win_box(window, 0, 0, t3_win_get_height(window), t3_win_get_width(window), colors.dialog_attrs);

	child_height = t3_win_get_height(window);
	if (!(dimension & COVER_TOP))
		child_height--;
	if (!(dimension & COVER_BOTTOM))
		child_height--;
	child_width = t3_win_get_width(window);
	if (!(dimension & COVER_LEFT))
		child_width--;
	if (!(dimension & COVER_RIGHT))
		child_width--;

	result &= child->set_size(child_height, child_width);
	return result;
}

t3_window_t *frame_t::get_draw_window(void) {
	return window;
}

bool frame_t::accepts_focus(void) { return child->accepts_focus(); }
