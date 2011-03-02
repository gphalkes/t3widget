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
#ifndef BULLET_H
#define BULLET_H

#include "widgets/widgets.h"

class bullet_t : public widget_t {
	private:
		bullet_status_t *source;
		int top, left;
		bool focus;

	public:
		bullet_t(container_t *parent, window_component_t *anchor,
			int _top, int _left, int relation, bullet_status_t *_source);
		virtual void process_key(key_t key);
		virtual bool resize(optint height, optint width, optint _top, optint _left);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
};

#endif
