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
#include <key/key.h>
#include <transcript.h>
#include <cerrno>
#include <new>
#include <signal.h>

#include "main.h"
#include "log.h"
#include "colorscheme.h"
#include "dialogs/dialog.h"
#include "textline.h"
#include "internal.h"

using namespace std;
using namespace sigc;

namespace t3_widget {
#define MESSAGE_DIALOG_WIDTH 50
#define MIN_LINES 16
#define MIN_COLUMNS 60

static int init_level;
static char *term_string;
static int screen_lines, screen_columns;
static signal<void, int, int> resize;
static signal<void> update_notification;

insert_char_dialog_t *insert_char_dialog;
message_dialog_t *message_dialog;

complex_error_t::complex_error_t(void) : success(true), source(SRC_NONE), error(0) {}
complex_error_t::complex_error_t(source_t _source, int _error) : success(false), source(_source), error(_error) {}

void complex_error_t::set_error(source_t _source, int _error) {
	success = false;
	source = _source;
	error = _error;
}

bool complex_error_t::get_success(void) { return success; }
complex_error_t::source_t complex_error_t::get_source(void) { return source; }
int complex_error_t::get_error(void) { return error; }

const char *complex_error_t::get_string(void) {
	switch (source) {
		case SRC_ERRNO:
			return strerror(error);
		case SRC_TRANSCRIPT:
			return transcript_strerror((transcript_error_t) error);
		case SRC_T3_KEY:
			return t3_key_strerror(error);
		case SRC_T3_WINDOW:
			return t3_window_strerror(error);
		default:
			return strerror(0);
	}
}

connection connect_resize(const slot<void, int, int> &slot) {
	return resize.connect(slot);
}

connection connect_update_notification(const slot<void> &slot) {
	return update_notification.connect(slot);
}

static signal<void> &on_init() {
	static signal<void> *on_init_obj = new signal<void>();
	return *on_init_obj;
}

/* We let connect_on_init return a bool, because we don't care about the actual
   sigc::connection and a bool takes less space. */
bool connect_on_init(const slot<void> &slot) {
	on_init().connect(slot);
	return true;
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
	int i;

	if (term_string == NULL)
		return;

	for (i = 0; terminal_mapping[i].name != NULL && strcmp(terminal_mapping[i].name, term_string) != 0; i++) {}
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

void restore(void) {
	switch (init_level) {
		case 3:
			cleanup_keys();
		case 2:
			terminal_specific_restore();
		case 1:
			/* Eat up all keys/terminal replies, before restoring the terminal. */
			while (t3_term_get_keychar(100) >= 0) {}
			t3_term_restore();
		case 0:
			free(term_string);
			term_string = NULL;
			break;
	}
	init_level = 0;
}

complex_error_t init(const char *term, bool separate_keypad) {
	complex_error_t result;
	int term_init_result;

	init_log();
	text_line_t::init();

	if (term == NULL) {
		const char *term_env = getenv("TERM");
		/* If term_env == NULL, t3_term_init will abort anyway, so we ignore
		   that case. */
		if (term_env != NULL)
			term_string = strdup(term_env);
	} else {
		term_string = strdup(term);
	}

	atexit(restore);
	if ((term_init_result = t3_term_init(-1, term)) != T3_ERR_SUCCESS) {
		restore();
		result.set_error(complex_error_t::SRC_T3_WINDOW, term_init_result);
		return result;
	}
	init_level++;

	terminal_specific_setup();
	init_level++;

	result = init_keys(term, separate_keypad);
	if (!result.get_success()) {
		restore();
		return result;
	}
	init_level++;

	init_colors();
	do_resize();
	try {
		/* Construct these here, such that the locale is set correctly and
		   gettext therefore returns the correctly localized strings. */
		if (message_dialog == NULL)
			message_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, _("Message"), _("Ok;oO"), NULL);
		if (insert_char_dialog == NULL)
			insert_char_dialog = new insert_char_dialog_t();
		on_init()();
	} catch (bad_alloc &ba) {
		restore();
		result.set_error(complex_error_t::SRC_ERRNO, ENOMEM);
	}
	return result;
}

void iterate(void) {
	key_t key;

	for (dialogs_t::iterator iter = dialog_t::active_dialogs.begin();
			iter != dialog_t::active_dialogs.end(); iter++)
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
		case EKEY_UPDATE_TERMINAL:
			break;
		default:
			//FIXME: pass unhandled keys to callback?
			dialog_t::active_dialogs.back()->process_key(key);
			break;
	}
}

void main_loop(void) {
	while (true) {
		iterate();
	}
}

void suspend(void) {
	//FIXME: check return values!
	deinit_keys();
	terminal_specific_restore();
	t3_term_restore();
	kill(getpid(), SIGSTOP);
	t3_term_init(-1, NULL);
	terminal_specific_setup();
	reinit_keys();
	do_resize();
}

void redraw(void) {
	do_resize();
	t3_term_redraw();
}

}; // namespace
