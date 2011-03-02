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
#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include "dialogs/dialogs.h"
#include "widgets/widgets.h"

#define MESSAGEDIALOG_MAX_LINES 10


class message_dialog_base_t : public dialog_t {
	private:
		int break_positions[MESSAGEDIALOG_MAX_LINES + 1];
		int height;
		line_t *message;
		const char *title;

		virtual void draw_dialog(void);

	public:
		message_dialog_base_t(int width, int top, int left, const char *_title);
		virtual bool resize(optint height, optint width, optint top, optint left);
		void set_message(const string *_message);
};


class message_dialog_t : public message_dialog_base_t {
	private:
		button_t *button;

	public:
		message_dialog_t(int width, int top, int left, const char *_title);
		virtual bool resize(optint height, optint width, optint top, optint left);
};

class question_dialog_t : public message_dialog_base_t {
	private:
		button_t *ok_button, *cancel_button;
		void *data;

	public:
		question_dialog_t(int width, int top, int left, const char *_title, const char *okName, const char *cancelName);
		virtual bool resize(optint height, optint width, optint top, optint left);

	SIGNAL(ok, void);
	SIGNAL(cancel, void);
};

#endif