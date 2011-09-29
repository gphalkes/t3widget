/* Copyright (C) 2011 G.P. Halkes
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
#include <cerrno>
#include <new>
#include <signal.h>
#include <pthread.h>
#include <t3key/key.h>
#include <transcript/transcript.h>
#ifdef __linux__
#include <sys/ioctl.h>
#include <linux/kd.h>
#endif

#include "main.h"
#include "log.h"
#include "colorscheme.h"
#include "dialogs/dialog.h"
#include "textline.h"
#include "internal.h"

using namespace std;

namespace t3_widget {
#define MESSAGE_DIALOG_WIDTH 50
#define MIN_LINES 16
#define MIN_COLUMNS 60

static int init_level;
static int screen_lines, screen_columns;
static sigc::signal<void, int, int> resize;
static sigc::signal<void> update_notification;
static pthread_mutex_t transcript_mutex = PTHREAD_MUTEX_INITIALIZER;

init_parameters_t *init_params;

#ifdef __linux__
static int linux_meta_mode = -1;
#endif

insert_char_dialog_t *insert_char_dialog;
message_dialog_t *message_dialog;

complex_error_t::complex_error_t(void) : success(true), source(SRC_NONE), error(0) {}
complex_error_t::complex_error_t(source_t _source, int _error, const char *_file_name, int _line_number) :
		success(false), source(_source), error(_error), file_name(_file_name), line_number(_line_number)
{}

void complex_error_t::set_error(source_t _source, int _error, const char *_file_name, int _line_number) {
	success = false;
	source = _source;
	error = _error;
	file_name = _file_name;
	line_number = _line_number;
}

bool complex_error_t::get_success(void) { return success; }
complex_error_t::source_t complex_error_t::get_source(void) { return source; }
int complex_error_t::get_error(void) { return error; }

const char *complex_error_t::get_string(void) {
	static string error_str;

	switch (source) {
		case SRC_ERRNO:
			if (file_name != NULL) {
				char number_buffer[128];
				error_str = file_name;
				sprintf(number_buffer, ":%d: ", line_number);
				error_str.append(number_buffer);
			} else {
				error_str = "tilde: ";
			}
			error_str.append(strerror(error));
			break;
		case SRC_TRANSCRIPT:
			error_str = "libtranscript: ";
			error_str.append(transcript_strerror((transcript_error_t) error));
			break;
		case SRC_T3_KEY:
			error_str = "libt3key: ";
			error_str.append(t3_key_strerror(error));
			break;
		case SRC_T3_WINDOW:
			error_str = "libt3window: ";
			error_str.append(t3_window_strerror(error));
			break;
		default:
			return strerror(0);
	}
	return error_str.c_str();
}

init_parameters_t *init_parameters_t::create(void) {
	return new init_parameters_t();
}

init_parameters_t::init_parameters_t(void) {
	separate_keypad = false;
	term = NULL;
	program_name = NULL;
}

sigc::connection connect_resize(const sigc::slot<void, int, int> &slot) {
	return resize.connect(slot);
}

sigc::connection connect_update_notification(const sigc::slot<void> &slot) {
	return update_notification.connect(slot);
}

static sigc::signal<void> &on_init() {
	static sigc::signal<void> *on_init_obj = new sigc::signal<void>();
	return *on_init_obj;
}

sigc::connection connect_on_init(const sigc::slot<void> &slot) {
	return on_init().connect(slot);
}

static sigc::signal<void> &terminal_settings_changed() {
	static sigc::signal<void> *terminal_settings_changed_obj = new sigc::signal<void>();
	return *terminal_settings_changed_obj;
}

sigc::connection connect_terminal_settings_changed(const sigc::slot<void> &slot) {
	return terminal_settings_changed().connect(slot);
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
	TERM_XTERM,
	TERM_LINUX
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
#ifdef __linux__
		case TERM_LINUX:
			if (linux_meta_mode > 0)
				ioctl(0, KDSKBMETA, linux_meta_mode);
			break;
#endif
		default:
			break;
	}
}

static void terminal_specific_setup(void) {
	int i;

	if (init_params->term == NULL)
		return;

	for (i = 0; terminal_mapping[i].name != NULL && strcmp(terminal_mapping[i].name, init_params->term) != 0; i++) {}
	terminal = terminal_mapping[i].code;

	switch (terminal) {
		case TERM_XTERM:
			t3_term_putp("\033[?1036s\033[?1036h");
			break;
#ifdef __linux__
		case TERM_LINUX:
			if (ioctl(0, KDGKBMETA, &linux_meta_mode) < 0)
				linux_meta_mode = -1;
			else
				ioctl(0, KDSKBMETA, K_ESCPREFIX);
			break;
#endif
		default:
			return;
	}
	atexit(terminal_specific_restore);
}

void restore(void) {
	switch (init_level) {
		default:
		case 3:
			cleanup_keys();
		case 2:
			terminal_specific_restore();
		case 1:
			if (init_level < 3) {
				/* Eat up all keys/terminal replies, before restoring the terminal. */
				while (t3_term_get_keychar(100) >= 0) {}
			}
			t3_term_restore();
		case 0:
			free(const_cast<char *>(init_params->term));
			init_params->term = NULL;
			free(const_cast<char *>(init_params->program_name));
			break;
	}
	init_level = 0;
}

complex_error_t init(const init_parameters_t *params) {
	complex_error_t result;
	int term_init_result;

	init_log();
	text_line_t::init();

	if (init_params == NULL)
		init_params = init_parameters_t::create();

	if (params == NULL || params->term == NULL) {
		const char *term_env = getenv("TERM");
		/* If term_env == NULL, t3_term_init will abort anyway, so we ignore
		   that case. */
		if (term_env != NULL)
			init_params->term = strdup(term_env);
	} else {
		init_params->term = strdup(params->term);
	}

	if (params != NULL) {
		init_params->separate_keypad = params->separate_keypad;
		init_params->program_name = strdup(params->program_name == NULL ? "This program" : params->program_name);
	}

	atexit(restore);
	if ((term_init_result = t3_term_init(-1, init_params->term)) != T3_ERR_SUCCESS) {
		restore();
		result.set_error(complex_error_t::SRC_T3_WINDOW, term_init_result);
		return result;
	}
	init_level++;

	terminal_specific_setup();
	init_level++;

	transcript_set_lock_callbacks((void (*)(void *)) pthread_mutex_lock, (void (*)(void *)) pthread_mutex_unlock, &transcript_mutex);

	result = init_keys(init_params->term, init_params->separate_keypad);
	if (!result.get_success()) {
		restore();
		return result;
	}
	init_level++;

	init_attributes();
	do_resize();
	try {
		/* Construct these here, such that the locale is set correctly and
		   gettext therefore returns the correctly localized strings. */
		if (message_dialog == NULL)
			message_dialog = new message_dialog_t(MESSAGE_DIALOG_WIDTH, _("Message"), _("Close"), NULL);
		if (insert_char_dialog == NULL)
			insert_char_dialog = new insert_char_dialog_t();
		on_init()();
	} catch (bad_alloc &ba) {
		restore();
		result.set_error(complex_error_t::SRC_ERRNO, ENOMEM);
	}
	t3_term_hide_cursor();
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
			terminal_settings_changed()();
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
	printf("%s has been stopped. You can return to %s by entering 'fg'.\n",
		init_params->program_name, init_params->program_name);
	kill(getpid(), SIGSTOP);
	t3_term_init(-1, NULL);
	terminal_specific_setup();
	reinit_keys();
	do_resize();
	t3_term_hide_cursor();
}

void redraw(void) {
	do_resize();
	t3_term_redraw();
}

long get_version(void) {
	return T3_WIDGET_VERSION;
}

long get_libt3key_version(void) {
	return t3_key_get_version();
}

long get_libt3window_version(void) {
	return t3_window_get_version();
}

}; // namespace
