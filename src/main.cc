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

#include <cstdlib>
#include <cstring>

#include "main.h"
#include "log.h"
#include "colorscheme.h"
#include "dialogs/dialog.h"
#include "textline.h"

using namespace std;
using namespace sigc;

namespace t3_widget {
#define MESSAGE_DIALOG_WIDTH 50
#define MIN_LINES 16
#define MIN_COLUMNS 60

static int screen_lines, screen_columns;
static signal<void, int, int> resize;
static signal<void> update_notification;

insert_char_dialog_t insert_char_dialog;

connection connect_resize(const slot<void, int, int> &slot) {
	return resize.connect(slot);
}

connection connect_update_notification(const slot<void> &slot) {
	return update_notification.connect(slot);
}

static void do_resize(void) {
	int new_screen_lines, new_screen_columns;

	t3_term_resize();
	t3_term_get_size(&new_screen_lines, &new_screen_columns);
	if (new_screen_lines == screen_lines && new_screen_columns == screen_columns)
		return;

	// Ensure minimal coordinates to maintain sort of sane screen layout
	screen_lines = new_screen_lines < MIN_LINES ? MIN_LINES : new_screen_lines;
	screen_columns = new_screen_columns < MIN_COLUMNS? MIN_COLUMNS : new_screen_columns;

	resize(screen_lines, screen_columns);
}

enum terminal_code_t {
	TERM_NONE,
	TERM_XTERM
};

struct terminal_mapping_t {
	const char *name;
	terminal_code_t code;
};

static terminal_mapping_t terminal_mapping[] = {
	{"xterm", TERM_XTERM},
	{NULL, TERM_NONE}
};

static terminal_code_t terminal;

static void terminal_specific_restore(void) {
	switch (terminal) {
		case TERM_XTERM:
			/* Note: this may not actually reset to previous value, because of broken xterm */
			t3_term_putp("\033[?1036r");
			break;
		default:
			break;
	}
}

static void terminal_specific_setup(void) {
	const char *term;
	int i;

	term = getenv("TERM");
	for (i = 0; terminal_mapping[i].name != NULL && strcmp(terminal_mapping[i].name, term) != 0; i++) {}
	terminal = terminal_mapping[i].code;

	switch (terminal) {
		case TERM_XTERM:
			t3_term_putp("\033[?1036s\033[?1036h");
			break;
		default:
			return;
	}
	atexit(terminal_specific_restore);
}

#warning FIXME: returning the value from t3_term_init is not very useful!
int init(main_window_base_t *main_window) {
	int result;

	init_log();
	init_colors(); // Probably called somewhere else already, but just to make sure
	text_line_t::init();

	if ((result = t3_term_init(-1, NULL)) != T3_ERR_SUCCESS) {
		t3_term_restore();
		return result;
	}
	atexit(t3_term_restore);

	terminal_specific_setup();
	atexit(terminal_specific_restore);

	init_keys();
	do_resize();
	dialog_t::init(main_window);
	return 0;
}

void iterate(void) {
	key_t key;

	for (dialogs_t::iterator iter = dialog_t::dialogs.begin(); iter != dialog_t::dialogs.end(); iter++)
		(*iter)->update_contents();

	t3_term_update();
	key = read_key();
	lprintf("Got key %04X\n", key);
	switch (key) {
		case EKEY_RESIZE:
			do_resize();
			break;
		case EKEY_EXTERNAL_UPDATE:
			update_notification();
			break;
		case EKEY_META | EKEY_ESC:
#warning FIXME: how do we handle the whole alt-message mess?
			//activate_window(WindowID::ALTMESSAGE_DIALOG);
			break;
		default:
			dialog_t::dialogs.back()->process_key(key);
			break;
	}
}

void main_loop(void) {
	while (true) {
		iterate();
	}
}

}; // namespace
