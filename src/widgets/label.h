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
#ifndef T3_WIDGET_LABEL_H
#define T3_WIDGET_LABEL_H

#include "widgets/widget.h"

namespace t3_widget {

class T3_WIDGET_API label_t : public widget_t {
	public:
		enum align_t {
			ALIGN_LEFT,
			ALIGN_RIGHT,
			ALIGN_LEFT_UNDERFLOW,
			ALIGN_RIGHT_UNDERFLOW,
			ALIGN_CENTER
		};

	private:
		std::string text;
		int width, text_width;
		align_t align;
		bool focus, can_focus;

	public:
		label_t(const char *_text);

		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);

		void set_align(align_t _align);
		void set_text(const char *_text);

		void set_accepts_focus(bool _can_focus);
		virtual bool accepts_focus(void);

};

}; // namespace
#endif

