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

#warning FIXME: allow anchor
multi_widget_t::multi_widget_t(container_t *parent, int _width, int _top, int _left, int relation) :
	width(_width), top(_top), left(_left), fixed_sum(0), proportion_sum(0)
{
	if ((window = t3_win_new_unbacked(parent->get_draw_window(), 1, width, top, left, 0, parent->get_draw_window(), relation)) == NULL)
		throw bad_alloc();
	t3_win_show(window);
}

void multi_widget_t::process_key(key_t key) {
	for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
		if (iter->send_keys)
			iter->widget->process_key(key);
	}
}

bool multi_widget_t::resize(optint height, optint _width, optint _top, optint _left) {
	(void) height;

	if (_top.is_valid())
		top = _top;
	if (_left.is_valid())
		left = _left;
	t3_win_move(window, top, left);

	if (_width.is_valid() && width != _width) {
		width = _width;
		resize_widgets();
	}
	return true; //FIXME: use result of widgets
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

/* Width is negative for fixed width widgets, positive for proportion */
void multi_widget_t::push_back(widget_t *widget, int _width, bool takes_focus, bool send_keys) {
	item_t item;

	if (takes_focus && !widget->accepts_focus())
		takes_focus = false;

	if (_width < 0) {
		widget->resize(None, -_width, None, None);
		fixed_sum += -_width;
	} else {
		proportion_sum += _width;
	}

	item.widget = widget;
	item.width = _width;
	item.takes_focus = takes_focus;
	item.send_keys = send_keys;
	widgets.push_back(item);
	resize_widgets();
}

void multi_widget_t::pop_back(void) {
	item_t widget = widgets.back();
	if (widget.width < 0)
		fixed_sum -= -widget.width;
	else
		proportion_sum -= widget.width;
	widgets.pop_back();
	resize_widgets();
}

widget_t *multi_widget_t::back(void) {
	return widgets.back().widget;
}

bool multi_widget_t::empty(void) {
	return widgets.empty();
}

void multi_widget_t::resize_widgets(void) {
	if (width <= fixed_sum) {
		for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
			if (iter->width < 0)
				continue;
			iter->widget->resize(None, 1, None, None);
		}
	} else if (proportion_sum > 0) {
		double scale = (double) (width - fixed_sum) / proportion_sum;
		int size;

		for (list<item_t>::iterator iter = widgets.begin(); iter != widgets.end(); iter++) {
			if (iter->width < 0)
				continue;
			size = scale * iter->width;
			if (size == 0)
				size = 1;
			//FIXME: make sure that widgets fill space available
			iter->widget->resize(None, size, None, None);
		}
	}
}
