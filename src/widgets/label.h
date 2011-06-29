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

#include <t3widget/widgets/widget.h>

namespace t3_widget {

/** A widget displaying a text.

    This widget can display a text. An alignment can be specified. For
    displaying labels for checkboxes and text fields, see smart_label_t.
*/
class T3_WIDGET_API label_t : public widget_t {
	public:
		/** Constants defining alignments. */
		enum align_t {
			ALIGN_LEFT, /**< Align text to the left of the allocated space. */
			ALIGN_RIGHT, /**< Align text to the right of the allocated space. */
			/** Align text to the left of the allocated space, with underflow indicator.
			    Similar to #ALIGN_LEFT, but if the allocated space is too small, the
			    text is cut off on the left instead of the right and the first two
			    visible characters of text are replaced by "..".
			*/
			ALIGN_LEFT_UNDERFLOW,
			/** Align text to the right of the allocated space, with underflow indicator.
			    Similar to #ALIGN_RIGHT, but if the allocated space is too small, the
			    first two visible characters of text are replaced by "..".
			*/
			ALIGN_RIGHT_UNDERFLOW,
			ALIGN_CENTER /**< Center text in the allocated space. */
		};

	private:
		std::string text; /**< Text currently displayed. */
		int width, /**< Current display width. */
			text_width; /**< Width of the text if displayed in full. */
		align_t align; /**< Text alignment. Default is #ALIGN_LEFT. */
		bool focus, /**< Boolean indicating whether this label_t has the input focus. */
			can_focus; /**< Boolean indicating whether this label_t will accept the input focus. Default is @c true. */

	public:
		/** Create a new label_t. */
		label_t(const char *_text);

		virtual bool process_key(key_t key);
		virtual bool set_size(optint height, optint width);
		virtual void update_contents(void);
		virtual void set_focus(bool _focus);
		virtual bool accepts_focus(void);

		/** Set the alignment. */
		void set_align(align_t _align);
		/** Set the text. */
		void set_text(const char *_text);

		/** Set whether this label_t accepts the input focus. */
		void set_accepts_focus(bool _can_focus);
};

}; // namespace
#endif

