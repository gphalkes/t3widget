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
#include "widgets/multiwidget.h"
#include "log.h"
using namespace std;
namespace t3_widget {

multi_widget_t::multi_widget_t(container_t *parent) : width(1), fixed_sum(0), proportion_sum(0), send_key_widget(NULL) {
	if ((window = t3_win_new_unbacked(parent->get_draw_window(), 1, width, 0, 0, 0)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
}

multi_widget_t::~multi_widget_t(void) {
	for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		delete iter->widget;
}

bool multi_widget_t::process_key(key_t key) {
	if (send_key_widget != NULL)
		return send_key_widget->process_key(key);
	return false;
}

bool multi_widget_t::set_size(optint height, optint _width) {
	(void) height;
	if (_width.is_valid() && width != _width) {
		width = _width;
		t3_win_resize(window, 1, width);
		resize_widgets();
	}
	return true;   //FIXME: use result of widgets
}


void multi_widget_t::update_contents(void) {
	for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		iter->widget->update_contents();
}

void multi_widget_t::set_focus(bool focus) {
	for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
		if (iter->takes_focus)
			iter->widget->set_focus(focus);
	}
}

bool multi_widget_t::accepts_focus(void) {
	for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		if (iter->takes_focus)
			return true;
	return false;
}

void multi_widget_t::force_redraw(void) {
	for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++)
		iter->widget->force_redraw();
}

/* Width is negative for fixed width widgets, positive for proportion */
void multi_widget_t::push_back(widget_t *widget, int _width, bool takes_focus, bool send_keys) {
	item_t item;

	if (takes_focus && !widget->accepts_focus())
		takes_focus = false;

	if (_width < 0) {
		widget->set_size(None, -_width);
		fixed_sum += -_width;
	} else {
		proportion_sum += _width;
	}

	item.widget = widget;
	item.width = _width;
	item.takes_focus = takes_focus;

	if (send_keys && send_key_widget == NULL) {
		focus_widget_t *focus_widget;
		send_key_widget = widget;
		if ((focus_widget = dynamic_cast<focus_widget_t *>(widget)) != NULL) {
			focus_widget->connect_move_focus_left(move_focus_left.make_slot());
			focus_widget->connect_move_focus_right(move_focus_right.make_slot());
			focus_widget->connect_move_focus_up(move_focus_up.make_slot());
			focus_widget->connect_move_focus_down(move_focus_down.make_slot());
		}
	}

	if (widgets.size() > 0)
		widget->set_anchor(widgets.back().widget, T3_PARENT(T3_ANCHOR_TOPRIGHT) | T3_CHILD(T3_ANCHOR_TOPLEFT));
	widget->set_position(0, 0);
	widgets.push_back(item);
	resize_widgets();
}

void multi_widget_t::resize_widgets(void) {
	if (proportion_sum > 0) {
		double scale = (double) (width - fixed_sum) / proportion_sum;
		int size;

		for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
			if (iter->width < 0)
				continue;
			iter->calculated_width = scale * iter->width;
			if (iter->calculated_width == 0)
				iter->calculated_width = 1;
			size = iter->calculated_width;
		}
		//FIXME: this will do for now, but should be slightly smarter
		if (size > width - fixed_sum) {
			for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end() && size > width - fixed_sum; iter++) {
				if (iter->width < 0)
					continue;
				if (iter->calculated_width > 1) {
					iter->calculated_width--;
					size--;
				}
			}
		} else if (size < width - fixed_sum) {
			for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end() && size < width - fixed_sum; iter++) {
				if (iter->width < 0)
					continue;
				iter->calculated_width++;
				size++;
			}
		}
		for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
			if (iter->width < 0)
				continue;
			iter->widget->set_size(1, iter->calculated_width);
		}
	}
}

}; // namespace
